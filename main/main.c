#include "function.h"
#include "../version"

#define WAKEUP_PIN 4
#define WAKEUP_LEVEL 1
#define WAKEUP_TIME (120)

//******************************************************************************************************************

void app_main()
{
    const char *TAGM = "MAIN";

    vTaskDelay(3000 / portTICK_RATE_MS);

//    esp_err_t esp_bluedroid_disable();
//    esp_err_t esp_bt_controller_disable();
//    esp_wifi_stop();
//    esp_wifi_deinit();


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

    //****************    SET WAKEUP PARAM    **************************
    gpio_num_t w_pin = WAKEUP_PIN;
    int w_level = WAKEUP_LEVEL;
    esp_sleep_enable_ext0_wakeup(w_pin, w_level);
    uint64_t time_in_us = WAKEUP_TIME * 1000000;
    esp_sleep_enable_timer_wakeup(time_in_us);
    //**************************************************************


    //****************    UART2 (LORA)    **************************
    evtq = xQueueCreate(10, sizeof(s_evt));//create a queue to handle uart event
    s_evt evt;
    serial_init();
    vTaskDelay(500 / portTICK_RATE_MS);
    lora_start = false;
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
    char stk[128]={0};//, stz[128]={0};
    struct tm *dtimka;
    time_t dit_ct;
    uint8_t col = 0, row = 0, blk = 0, cnt = 0xff, don=true;
    uint32_t kol=0;
    TickType_t adc_tw = 0, wst = 0;
    const uint8_t sub_val = 5;
    const TickType_t sub_tmr = 100;

    vTaskDelay(1000 / portTICK_RATE_MS);
    ssd1306_contrast(cnt);
    ssd1306_clear();
    //**************************************************************


    while (true) {

	if (don) {
	    if (check_tmr(adc_tw)) {
		adc_tw = get_tmr(1000);
		dit_ct = time(NULL);
		dtimka=localtime(&dit_ct);
		memset(stk,0,128);
		col = calcx(sprintf(stk,"%02d:%02d:%02d", dtimka->tm_hour, dtimka->tm_min, dtimka->tm_sec));
		get_tsensor(&tc);
		sprintf(stk+strlen(stk),"\nChip : %.1fv %d%cC", (double)tc.vcc/1000, (int)round(tc.cels), 0x1F);
		ssd1306_text_xy(stk, col, 1);
	    }
	}

	if (xQueueReceive(evtq, &evt, 10/portTICK_RATE_MS) == pdTRUE) {
	    ssd1306_on(); //display on
	    don = true;
	    ssd1306_contrast(cnt);
	    memset(stk,0,128);
	    if (!evt.type) {
		sprintf(stk,"Send");
		row = 7;
	    } else {
		sprintf(stk,"Recv");
		row = 8;
	    }
	    sprintf(stk+strlen(stk)," pack #%u", evt.num);
	    col = calcx(strlen(stk));
	    ssd1306_text_xy(stk, col, row);

	    if (!blk) {// for start contrast play
		blk = 1; kol = 0; wst = get_tmr(sub_tmr);
		memset(stk,0,128);
		col = calcx(sprintf(stk,"Goto sleep..."));
		ssd1306_text_xy(stk, col, 5);
	    }
	}

	if (blk) {//contrast play
	    if (check_tmr(wst)) {
		kol++;
		if (cnt >= sub_val) cnt -= sub_val; else cnt = 0;
		ssd1306_contrast(cnt);
		//memset(stz,0,128); sprintf(stz,"Contrast value=%u iter=%u\n\n", cnt, kol); printik(TAGM, stz, GREEN_COLOR);
		if (!cnt) {
		    cnt = 0xff; blk = ~cnt; // stop contrast play
		    ssd1306_off(); //display off
		    don = false;
		}
		wst = get_tmr(sub_tmr);
	    }
	}

	if (!don) {
	    while (lora_start) vTaskDelay(100 / portTICK_RATE_MS);
	    memset(stk,0,128); sprintf(stk,"Goto light-sleep mode\n\n"); printik(TAGM, stk, GREEN_COLOR);
	    esp_light_sleep_start();//goto sleep mode

	    memset(stk,0,128);
	    esp_sleep_wakeup_cause_t ca = esp_sleep_get_wakeup_cause();
	    switch (ca) {
		case ESP_SLEEP_WAKEUP_EXT0:
		case ESP_SLEEP_WAKEUP_EXT1:
		    sprintf(stk,"Wakeup from GPIO %d level %d !!!\n", w_pin, w_level); 
		break;
		case ESP_SLEEP_WAKEUP_TIMER:
		    sprintf(stk,"Wakeup from timer (%u sec) !!!\n", WAKEUP_TIME);
		break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD:
		    sprintf(stk,"Wakeup from touchpad !!!\n");
		break;
		case ESP_SLEEP_WAKEUP_ULP:
		    sprintf(stk,"Wakeup from ULP !!!\n");
		break;
		    default : sprintf(stk,"Wakeup from undefined source (%u) !!!\n", ca);
	    }
	    printik(TAGM, stk, GREEN_COLOR);

	    ssd1306_on(); don = true;
	    ssd1306_contrast(cnt);
	    if (lora_start == false) {
		if (xTaskCreate(&serial_task, "serial_task", 4096, NULL, 7, NULL) != pdPASS) {
		    ESP_LOGE(TAGM, "Create serial_task failed | FreeMem %u", xPortGetFreeHeapSize());
		}
	    }
	}

    }

//    if (macs) free(macs);
//    esp_restart();

}
