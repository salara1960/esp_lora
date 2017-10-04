#include "function.h"
#include "../version"


uint8_t *macs = NULL;
static char sta_mac[18] = {0};
uint32_t cli_id = 0;
static const char *TAGM = "MAIN";
xQueueHandle evtq = NULL;

//******************************************************************************************************************

inline void get_tsensor(t_sens_t *t_s)
{
    t_s->vcc = (uint32_t)(adc1_get_raw(ADC1_TEST_CHANNEL) * 0.8);
    t_s->faren = temprature_sens_read() - 40;
    t_s->cels = (t_s->faren - 32) * 5/9;
}

//******************************************************************************************************************

void app_main()
{

    vTaskDelay(2000 / portTICK_RATE_MS);

    macs = (uint8_t *)calloc(1, 6);
    if (macs) {
        esp_efuse_mac_get_default(macs);
        sprintf(sta_mac, MACSTR, MAC2STR(macs));
        memcpy(&cli_id, &macs[2], 4);
        cli_id = ntohl(cli_id);
    }
    ets_printf("\n\nDevID %X | Appication version %s | SDK Version %s | FreeMem %u\n", cli_id, Version, esp_get_idf_version(), xPortGetFreeHeapSize());



    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN_11db);



    //****************    UART2 (LORA)    **************************
    evtq = xQueueCreate(10, sizeof(s_evt));//create a queue to handle uart event
    s_evt evt;
    serial_init();
    vTaskDelay(500 / portTICK_RATE_MS);
    if (xTaskCreate(&serial_task, "serial_task", 2048, NULL, 7, NULL) != pdPASS) {
	ESP_LOGE(TAGM, "Create serial_task failed | FreeMem %u", xPortGetFreeHeapSize());
    }
    //**************************************************************


    //*********************    SSD1306    **************************
    i2c_ssd1306_init();
    vTaskDelay(500 / portTICK_RATE_MS);
    ssd1306_off();
    vTaskDelay(1000 / portTICK_RATE_MS);

    esp_err_t ssd_ok = ssd1306_init();
    if (ssd_ok == ESP_OK) ssd1306_pattern();

    t_sens_t tc;
    char stk[128]={0}, stz[128]={0};
    struct tm *dtimka;
    int tu, tn, di_hour, di_min, di_sec;//, di_day, di_mon;//, di_year;
    time_t dit_ct;
    uint8_t clr = 0, row = 0;//, inv_cnt = 30;
    uint32_t adc_tw = get_tmr(1000);
    //**************************************************************



    while (true) {

	if (check_tmr(adc_tw)) {
	    if (ssd_ok == ESP_OK) {
		//inv_cnt--;
		//if (!inv_cnt) {
		//    inv_cnt = 30;
		//    ssd1306_invert();
		//}
		if (!clr) {
		    ssd1306_clear();
		    clr = 1;
		}
		dit_ct = time(NULL);
		dtimka=localtime(&dit_ct);
		di_hour=dtimka->tm_hour;	di_min=dtimka->tm_min;	di_sec=dtimka->tm_sec;
		//di_day=dtimka->tm_mday;	di_mon=dtimka->tm_mon+1;	//di_year=dtimka->tm_year+1900;
		memset(stk,0,128);
		memset(stz,0,128);
		sprintf(stz,"%02d:%02d:%02d", di_hour, di_min, di_sec);
		tu = strlen(stz);
		if ((tu > 0) && (tu <= 16)) {
		    tn = (16 - tu ) / 2;
		    if ((tn > 0) && (tn < 8)) sprintf(stk+strlen(stk),"%*.s",tn," ");
		    sprintf(stk+strlen(stk),"%s\n", stz);
		}
		get_tsensor(&tc);
		sprintf(stk+strlen(stk),"Chip : %.1fv %d%cC", (double)tc.vcc/1000, (int)round(tc.cels), 0x1F);
		ssd1306_text_xy(stk, 2, 1);
	    }
	    adc_tw = get_tmr(1000);
	}
	if (xQueueReceive(evtq, &evt, 25/portTICK_RATE_MS) == pdTRUE) {
	    ssd1306_invert();
	    memset(stz,0,128);
	    if (!evt.type) {
		sprintf(stz,"Send");
		row = 8;
	    } else {
		sprintf(stz,"Recv");
		row = 7;
	    }
	    sprintf(stz+strlen(stz)," pack #%u", evt.num);
	    tu = strlen(stz); tn = 0;
	    if ( (tu > 0) && (tu <= 16) ) tn = ((16 - tu) / 2) + 1;
	    ssd1306_text_xy(stz, (uint8_t)tn, row);
	}

    }

//    if (macs) free(macs);
//    esp_restart();

}
