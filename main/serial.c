
#include "at_cmd.c"

const char *TAG_UART = "UART";
const int unum = UART_NUMBER;
const int uspeed = UART_SPEED;
const int BSIZE = 128;
const uint32_t wait_ack = 2000;
s_pctrl pctrl = {0, 0, 1, 1, 0};
bool lora_start = false, ts_set = false;
static uint8_t allcmd=0;
static uint32_t pknum_tx=0;
static uint32_t pknum_rx=0;
static bool mode = false;//at_command

//******************************************************************************************

void serial_init()
{
    uart_config_t uart_conf = {
        .baud_rate = uspeed,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    uart_param_config(unum, &uart_conf);

    uart_set_pin(unum, GPIO_U2TXD, GPIO_U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);//Set UART2 pins(TX, RX, RTS, CTS)

    uart_driver_install(unum, BSIZE << 1, 0, 0, NULL, 0);
}
//-----------------------------------------------------------------------------------------
void lora_init()
{
    gpio_pad_select_gpio(U2_CONFIG); /*gpio_pad_pulldown(U2_CONFIG);*/ gpio_set_direction(U2_CONFIG, GPIO_MODE_OUTPUT);
    gpio_set_level(U2_CONFIG, pctrl.config);//0 //set configure mode - at_command
    gpio_pad_select_gpio(U2_SLEEP); /*gpio_pad_pulldown(U2_SLEEP);*/ gpio_set_direction(U2_SLEEP, GPIO_MODE_OUTPUT);
    gpio_set_level(U2_SLEEP, pctrl.sleep);//0 //set no sleep
    gpio_pad_select_gpio(U2_STATUS); /*gpio_pad_pullup(U2_STATUS);*/ gpio_set_direction(U2_RESET, GPIO_MODE_INPUT);
    pctrl.status = gpio_get_level(U2_STATUS);
    gpio_pad_select_gpio(U2_RESET); /*gpio_pad_pullup(U2_RESET);*/ gpio_set_direction(U2_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level(U2_RESET, pctrl.reset);//1 //set no reset
}
//-----------------------------------------------------------------------------------------
void lora_reset()
{
    if (!(pctrl.status = gpio_get_level(U2_STATUS))) vTaskDelay(50 / portTICK_RATE_MS);//wait status=high (20 ms)

    pctrl.reset = 0; gpio_set_level(U2_RESET, pctrl.reset);//set reset
    vTaskDelay(1 / portTICK_RATE_MS);
    pctrl.reset = 1; gpio_set_level(U2_RESET, pctrl.reset);//set no reset
}
//-----------------------------------------------------------------------------------------
void lora_data_mode(bool cnf)//true - data mode, false - at_command mode
{
    if (cnf) pctrl.config = 1; else pctrl.config = 0;
    gpio_set_level(U2_CONFIG, pctrl.config);//0 - configure mode (at_command), 1 - data transfer mode
}
//-----------------------------------------------------------------------------------------
bool lora_at_mode()//true - at_command mode, false - data mode
{
    return (pctrl.config);
}
//-----------------------------------------------------------------------------------------
bool lora_sleep_mode(bool slp)//true - sleep mode, false - normal mode
{
    if (slp) {
	if (pctrl.sleep) return (bool)pctrl.sleep;//already sleep mode
	pctrl.sleep = 1;
    } else {
	if (!pctrl.sleep) return (bool)pctrl.sleep;//already normal mode
	pctrl.sleep = 0;
    }
    gpio_set_level(U2_SLEEP, pctrl.sleep);

    if (!pctrl.sleep) {
	uint8_t sch = 12;
	while ( sch-- && !(pctrl.status = gpio_get_level(U2_STATUS)) ) vTaskDelay(1 / portTICK_RATE_MS);
    }

    return (bool)pctrl.sleep;
}
//-----------------------------------------------------------------------------------------
bool lora_check_sleep()
{
    return (bool)pctrl.sleep;
}
//-----------------------------------------------------------------------------------------
bool lora_check_status()
{
    pctrl.status = gpio_get_level(U2_STATUS);

    return (bool)pctrl.status;
}
//-----------------------------------------------------------------------------------------
void serial_task(void *arg)
{
lora_start = true;
char stx[256]={0};
char *uks=NULL, *uke=NULL;


    sprintf(stx, "Start serial_task | FreeMem %u\n", xPortGetFreeHeapSize()); printik(TAG_UART, stx, CYAN_COLOR);

    char *data = (char *)calloc(1, BSIZE + 1);
    if (data) {
	if (!pknum_tx) {
	    lora_init();
	    vTaskDelay(50 / portTICK_RATE_MS);
	}
	lora_reset();
	vTaskDelay(100 / portTICK_RATE_MS);

	char cmds[BSIZE], tmp[32]={0};
	uint32_t len=0, dl=0;
	uint8_t buf=0, rd_done=0;
	bool needs=false;
	TickType_t tms=0, tmneeds=0;
	t_sens_t tchip;
	s_evt evt;
	memset(&lora_stat, 0, sizeof(s_lora_stat));

	while (true) {

	    while (allcmd < TotalCmd) {//at command loop

		if (!allcmd) ets_printf("%s[%s] Wait init lora module...%s\n", GREEN_COLOR, TAG_UART, STOP_COLOR);

		memset(cmds, 0, BSIZE);
		sprintf(cmds, "%s", at_cmd[allcmd].cmd);

//		if (!strcmp(cmds, "AT+LRSF=")) {
//		    sprintf(cmds+strlen(cmds),"C");//7—SF=7, 8—SF=8, 9—SF=9, A—SF=10, B—SF=11, C—SF=12
//		    lora_stat.sf = 12;
//		}
//		else
		//if (!strcmp(cmds, "AT+LRSBW=")) sprintf(cmds+strlen(cmds),"7");//6-62.5, 7-125, 8-250, 9-500
		//else
		//if (!strcmp(cmds, "AT+NODE=")) sprintf(cmds+strlen(cmds),"0,0");//AT+NODE=n,m -> n: 0—disable, 1—enable; mode: 0—only match NID, 1-match NID and BID
		//else
		//if (!strcmp(cmds, "AT+NID=")) sprintf(cmds+strlen(cmds),"0");//0xBC//In FSK mode. The node ID can be set 0..255
		//if (!strcmp(cmds, "AT+SYNW=")) sprintf(cmds+strlen(cmds),"C405EF90");//AT+SYNW=1234ABEF\r\n (if sync word is 0x12,0x34,0xAB,0xEF)
		//else
		//if (!strcmp(cmds, "AT+SYNL=")) sprintf(cmds+strlen(cmds),"4");//set sync word len // 0..8
		//else
		//if (!strcmp(cmds, "AT+POWER=")) sprintf(cmds+strlen(cmds),"2");//set POWER to 20dbm //0—20dbm//1—17dbm//2—15dbm//3—10dbm//4-???//5—8dbm//6—5dbm//7—2dbm
		//else
		//if (!strcmp(cmds, "AT+POWER=")) {
		//    sprintf(cmds+strlen(cmds),"0");//2//set POWER to //0—20dbm 5—8dbm, 1—17dbm 6—5dbm, 2—15dbm 7—2dbm, 3—10dbm
		//    lora_stat.power = 0;
		//} else
//		if (!strcmp(cmds, "AT+LRHF=")) {
//		    sprintf(cmds+strlen(cmds),"1");//0,1
//		    lora_stat.hfss = 1;
//		} else if (!strcmp(cmds, "AT+LRHPV=")) sprintf(cmds+strlen(cmds),"10");//0..255
//		else if (!strcmp(cmds, "AT+LRFSV=")) sprintf(cmds+strlen(cmds),"819");//0..65535  //819 - 50KHz (1638 - 100KHz)
//		else 
//		if (!strcmp(cmds, "AT+CS=")) {
//		    sprintf(cmds+strlen(cmds),"A");//B//set Channel Select to 10 //0..F — 0..15 channel
//		    lora_stat.chan = 10;
//		}
//		else
		if (strchr(cmds,'=')) sprintf(cmds+strlen(cmds),"?");


		sprintf(cmds+strlen(cmds),"\r\n");
		dl = strlen(cmds);
#ifdef PRINT_AT
		ets_printf("%s%s%s", BROWN_COLOR, cmds, STOP_COLOR);
#endif
		uart_write_bytes(unum, cmds, dl);//send at_command
		tms = get_tmr(at_cmd[allcmd].wait);
		rd_done = 0; len = 0; memset(data, 0, BSIZE);
		while ( !rd_done && !check_tmr(tms) ) {
		    if (uart_read_bytes(unum, &buf, 1, (TickType_t)25) == 1) {
			data[len++] = buf;
			if ( (strstr(data, "\r\n")) || (len >= BSIZE - 2) ) {
			    if (strstr(data, "ERROR:")) {
				ets_printf("%s%s%s", RED_COLOR, data, STOP_COLOR);
			    } else {
#ifdef PRINT_AT
				ets_printf("%s", data);
#endif
				if (data[0] == '+') put_at_value(allcmd, data);
			    }
			    rd_done = 1;
			}
		    }
		}
		allcmd++;
		vTaskDelay(10 / portTICK_RATE_MS);//50//200
	    }//while (allcmd < TotalCmd)

	    if (!mode) {//at_command mode
		if (lora_stat.plen) ets_printf("%s[%s] Freq=%s Mode=%s Hopping=%u Power=%s Channel=%u BandW=%s SF=%u PackLen=%u%s\n",
						GREEN_COLOR, TAG_UART,
						lora_freq[lora_stat.freq],
						lora_main_mode[lora_stat.mode],
						lora_stat.hfss,
						lora_power[lora_stat.power],
						lora_stat.chan,
						lora_bandw[lora_stat.bandw - 6],
						lora_stat.sf,
						lora_stat.plen,
						STOP_COLOR);
		mode = true;
		lora_data_mode(mode);//set data mode
		vTaskDelay(15 / portTICK_RATE_MS);
		len = 0; memset(data, 0, BSIZE);
		ets_printf("%s[%s] Device %X switch from at_command to data tx/rx mode and goto sleep%s\n", MAGENTA_COLOR, TAG_UART, cli_id, STOP_COLOR);
		needs = false;
	    } else {//data transfer mode
		if (!needs) {
		    get_tsensor(&tchip);
		    if (ts_set) {// time already set, send message without request timestamp and timezone
			dl = sprintf(cmds,"DevID %08X (%u): %.1fv %ddeg.C\r\n", cli_id, ++pknum_tx, (double)tchip.vcc/1000, (int)round(tchip.cels));
		    } else {// send message with request timestamp and timezone
			dl = sprintf(cmds,"DevID %08X (%u) TS: %.1fv %ddeg.C\r\n", cli_id, ++pknum_tx, (double)tchip.vcc/1000, (int)round(tchip.cels));
		    }
		    if (!lora_sleep_mode(false)) {}// !!! wakeup !!!
			//printik(TAG_UART, "Module wakeup and goto normal mode", MAGENTA_COLOR);

		    uart_write_bytes(unum, cmds, dl);
		    printik(TAG_UART, cmds, MAGENTA_COLOR);
		    evt.type = 0; evt.num = pknum_tx;
		    if (xQueueSend(evtq, (void *)&evt, (TickType_t)0) != pdPASS) {
			ESP_LOGE(TAG_UART,"Error while sending to evtq");
		    }

		    needs = true;
		    tmneeds = get_tmr(wait_ack);
		}
	    }

	    if (uart_read_bytes(unum, &buf, 1, (TickType_t)20) == 1) {
		data[len++] = buf;
		if ( (strchr(data, '\n')) || (len >= BSIZE - 2) ) {
		    if (!strchr(data,'\n')) sprintf(data,"\n");
		    memset(stx,0,256); sprintf(stx,"Recv (%u) : %s", ++pknum_rx, data); printik(TAG_UART, stx, BROWN_COLOR);
		    if (!ts_set) {
			uks = strstr(data, "TS[");
			if (uks) {
			    uks += 3;
			    uke = strchr(uks,':');
			    if (uke) {
				memset(tmp,0,32);
				memcpy(tmp, uks, (uint32_t)((uke-uks))&0x0F);//timestamp
				SNTP_SET_SYSTEM_TIME_US( (time_t)atoi(tmp), 0 );
				uks = uke + 1;
				uke = strchr(uks,']');
				if (uke) {
				    *uke = '\0';
				    setenv("TZ", uks, 1);
				    tzset();
				    ts_set = true;
				}
			    }
			}
		    }
		    len = 0; memset(data, 0, BSIZE);
		    evt.type = 1; evt.num = pknum_rx;
		    if (xQueueSend(evtq, (void *)&evt, (TickType_t)0) != pdPASS) {
		        ESP_LOGE(TAG_UART,"Error while sending to evtq");
		    }
		    if (needs) tmneeds = get_tmr(0);
		}
	    }

	    if (needs) {
		if (check_tmr(tmneeds)) {
		    if (lora_sleep_mode(true)) {
			//memset(stx,0,256); sprintf(stx,"Goto sleep mode until %u sec\n", sleep_time/1000); printik(TAG_UART, stx, MAGENTA_COLOR);
			break;
		    }
		    needs = false;
		}
	    }

	}

	free(data);

    }

    memset(stx,0,256); sprintf(stx, "Stop serial_task | FreeMem %u\n", xPortGetFreeHeapSize()); printik(TAG_UART, stx, CYAN_COLOR);

    lora_start = false;
    vTaskDelete(NULL);
}

//******************************************************************************************

