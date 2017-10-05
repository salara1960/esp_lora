#include "function.h"
#include "../version"

//******************************************************************************************************************

void app_main()
{
    const char *TAGM = "MAIN";

    vTaskDelay(2000 / portTICK_RATE_MS);

    uint8_t *macs = (uint8_t *)calloc(1, 6);
    if (macs) {
	char sta_mac[18] = {0};
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
    if (xTaskCreate(&serial_task, "serial_task", 4096, NULL, 7, NULL) != pdPASS) {
	ESP_LOGE(TAGM, "Create serial_task failed | FreeMem %u", xPortGetFreeHeapSize());
    }
    //**************************************************************


    //*********************    SSD1306    **************************
    i2c_ssd1306_init();
    vTaskDelay(500 / portTICK_RATE_MS);
    ssd1306_off();
    vTaskDelay(1000 / portTICK_RATE_MS);

    ssd1306_init();
    ssd1306_pattern();
    vTaskDelay(1000 / portTICK_RATE_MS);

    t_sens_t tc;
    char stk[128]={0}, stz[128]={0};
    struct tm *dtimka;
    int tu, tn, di_hour, di_min, di_sec;
    time_t dit_ct;
    uint8_t row = 0;
    TickType_t adc_tw = 0;

    vTaskDelay(1000 / portTICK_RATE_MS);
    ssd1306_clear();
    //**************************************************************


    while (true) {

	if (check_tmr(adc_tw)) {
	    dit_ct = time(NULL);
	    dtimka=localtime(&dit_ct);
	    di_hour=dtimka->tm_hour;	di_min=dtimka->tm_min;	di_sec=dtimka->tm_sec;
	    memset(stk,0,128);
	    tu = sprintf(stk,"%02d:%02d:%02d", di_hour, di_min, di_sec);
	    tn=0; if ( (tu > 0) && (tu < 16) ) tn = ((16 - tu) / 2) + 1;
	    get_tsensor(&tc);
	    sprintf(stk+strlen(stk),"\nChip : %.1fv %d%cC", (double)tc.vcc/1000, (int)round(tc.cels), 0x1F);
	    ssd1306_text_xy(stk, (uint8_t)tn, 1);
	    adc_tw = get_tmr(1000);
	} else vTaskDelay(100 / portTICK_RATE_MS);

	if (xQueueReceive(evtq, &evt, 10/portTICK_RATE_MS) == pdTRUE) {
	    //ssd1306_invert();
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
