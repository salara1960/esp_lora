
#include "at_cmd.c"

const char *TAG_UART = "UART";
const int unum = UART_NUMBER;
const int uspeed = UART_SPEED;
const int BSIZE = 128;
const uint32_t sleep_time = 120000, wait_txd = 1000;
s_pctrl pctrl = {0, 0, 1, 1, 0};

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
//	if (!(pctrl.status = gpio_get_level(U2_STATUS))) vTaskDelay(25 / portTICK_RATE_MS);//wait status=high (20 ms)
    }// else {
//	if ((pctrl.status = gpio_get_level(U2_STATUS))) vTaskDelay(25 / portTICK_RATE_MS);//wait status=low (20 ms)
//    }

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

    ets_printf("%s[%s] Start serial_task (%d) | FreeMem %u%s\n", START_COLOR, TAG_UART, TotalCmd, xPortGetFreeHeapSize(), STOP_COLOR);

    char *data = (char *)calloc(1, BSIZE + 1);
    if (data) {
	char cmds[BSIZE];
	char stx[256];
	lora_init();
	vTaskDelay(50 / portTICK_RATE_MS);
	lora_reset();
	vTaskDelay(100 / portTICK_RATE_MS);
	uint32_t len=0, dl=0, pknum_tx=0, pknum_rx=0;
	uint8_t buf=0, allcmd=0, rd_done=0;
	bool mode=false, needs=false;
	TickType_t tms=0, tmsend=0, tmneeds=0;
	t_sens_t tchip;
	s_evt evt;

	while (true) {
	    while (allcmd < TotalCmd) {//at command loop

		memset(cmds, 0, BSIZE);
		sprintf(cmds, "%s", at_cmd[allcmd].cmd);

		//if (!strcmp(cmds, "AT+LRSF=")) sprintf(cmds+strlen(cmds),"C");//7—SF=7, 8—SF=8, 9—SF=9, A—SF=10, B—SF=11, C—SF=12
		//else if (!strcmp(cmds, "AT+LRSBW=")) sprintf(cmds+strlen(cmds),"8");//6-62.5, 7-125, 8-250, 9-500
		//else 
		if (strchr(cmds,'=')) sprintf(cmds+strlen(cmds),"?");

		sprintf(cmds+strlen(cmds),"\r\n");
		dl = strlen(cmds);
		ets_printf("%s%s%s", BROWN_COLOR, cmds, STOP_COLOR);

		uart_write_bytes(unum, cmds, dl);//send at_command
		tms = get_tmr(at_cmd[allcmd].wait);
		rd_done = 0; len = 0; memset(data, 0, BSIZE);
		while ( !rd_done && !check_tmr(tms) ) {
		    if (uart_read_bytes(unum, &buf, 1, (TickType_t)25) == 1) {
			data[len++] = buf;
			if ( (strstr(data, "\r\n")) || (len >= BSIZE - 2) ) {
			    if (strstr(data, "ERROR:")) ets_printf("%s%s%s", RED_COLOR, data, STOP_COLOR);
						   else ets_printf("%s", data);
			    rd_done = 1;
			}
		    }
		}
		allcmd++;
		vTaskDelay(200 / portTICK_RATE_MS);
	    }//while (allcmd < TotalCmd)

	    if (!mode) {//at_command mode
		mode = true;
		lora_data_mode(mode);//set data mode
		vTaskDelay(15 / portTICK_RATE_MS);
		len = 0; memset(data, 0, BSIZE);

		lora_sleep_mode(true);// !!! set sleep mode !!!

		ets_printf("%s[%s] Device %X switch from at_command to data tx/rx mode and goto sleep%s\n", MAGENTA_COLOR, TAG_UART, cli_id, STOP_COLOR);
		tmsend = get_tmr(2000);
	    } else {//data transfer mode
		if (check_tmr(tmsend)) {
		    tmsend = get_tmr(sleep_time);
		    get_tsensor(&tchip);
		    dl = sprintf(cmds,"DevID %08X (%u): %.1f v %d deg.C\r\n", cli_id, ++pknum_tx, (double)tchip.vcc/1000, (int)round(tchip.cels));

		    if (!lora_sleep_mode(false))// !!! wakeup !!!
			//ets_printf("%s[%s] Module wakeup and goto normal mode%s\n", MAGENTA_COLOR, TAG_UART, STOP_COLOR);
			printik(TAG_UART, "Module wakeup and goto normal mode", MAGENTA_COLOR);

		    uart_write_bytes(unum, cmds, dl);
		    //ets_printf("%s[%s] Send : %s%s", MAGENTA_COLOR, TAG_UART, cmds, STOP_COLOR);
		    printik(TAG_UART, cmds, MAGENTA_COLOR);
		    evt.type = 0; evt.num = pknum_tx;
		    if (xQueueSend(evtq, (void *)&evt, (TickType_t)0) != pdPASS) {
			ESP_LOGE(TAG_UART,"Error while sending to evtq");
		    }

		    needs = true;
		    tmneeds = get_tmr(wait_txd);
		    //vTaskDelay(wait_txd / portTICK_RATE_MS);//wait while data transmiting
		    //if (lora_sleep_mode(true)) ets_printf("%s[%s] Goto sleep mode until %u sec%s\n", MAGENTA_COLOR, TAG_UART, sleep_time/1000, STOP_COLOR);

		}
	    }

//	    if (!lora_check_sleep()) {
		if (uart_read_bytes(unum, &buf, 1, (TickType_t)20) == 1) {
		    //ets_printf("%s[%s] 0x%02X%s\n", MAGENTA_COLOR, TAG_UART, buf, STOP_COLOR);
		    data[len++] = buf;
		    if ( (strchr(data, '\n')) || (len >= BSIZE - 2) ) {
			if (!strchr(data,'\n')) sprintf(data,"\n");
			//ets_printf("%s[%s] Recv (%u) : %s%s", BROWN_COLOR, TAG_UART, ++pknum_rx, data, STOP_COLOR);
			memset(stx,0,256); sprintf(stx,"Recv (%u) : %s", ++pknum_rx, data); printik(TAG_UART, stx, BROWN_COLOR);
			len = 0; memset(data, 0, BSIZE);
			evt.type = 1; evt.num = pknum_rx;
			if (xQueueSend(evtq, (void *)&evt, (TickType_t)0) != pdPASS) {
			    ESP_LOGE(TAG_UART,"Error while sending to evtq");
			}
			if (needs) tmneeds = get_tmr(0);
		    }
		}
//	    }

	    if (needs) {
		if (check_tmr(tmneeds)) {
		    if (lora_sleep_mode(true)) {
			//ets_printf("%s[%s] Goto sleep mode until %u sec%s\n", MAGENTA_COLOR, TAG_UART, sleep_time/1000, STOP_COLOR);
			memset(stx,0,256); sprintf(stx,"Goto sleep mode until %u sec\n\n", sleep_time/1000); printik(TAG_UART, stx, MAGENTA_COLOR);
		    }
		    needs = false;
		}
	    }

	    //vTaskDelay(25 / portTICK_RATE_MS);
	}

	vTaskDelay(500 / portTICK_RATE_MS);
	free(data);

    }

    ets_printf("%s[%s] Stop serial_task | FreeMem %u%s\n", START_COLOR, TAG_UART, xPortGetFreeHeapSize(), STOP_COLOR);

    vTaskDelete(NULL);
}

//******************************************************************************************

