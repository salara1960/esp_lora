#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

#include "esp_system.h"
#include "esp_adc_cal.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
//#include "nvs_flash.h"
//#include "nvs.h"
#include "esp_partition.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_wps.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include <esp_intr.h>
#include "esp_sleep.h"
#include "esp_types.h"

//#include "stdatomic.h"

//#include "tcpip_adapter.h"

#include "rom/ets_sys.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include <soc/rmt_struct.h>
#include <soc/dport_reg.h>
#include <soc/gpio_sig_map.h>

//#include "lwip/ip_addr.h"
//#include "lwip/err.h"
//#include "lwip/sockets.h"
//#include "lwip/sys.h"
//#include "lwip/netdb.h"
//#include "lwip/dns.h"
//#include "lwip/api.h"
//#include "lwip/netif.h"


#define BLACK_COLOR  "\x1B[30m"
#define RED_COLOR  "\x1B[31m"
#define GREEN_COLOR  "\x1B[32m"
#define BROWN_COLOR  "\x1B[33m"
#define BLUE_COLOR  "\x1B[34m"
#define MAGENTA_COLOR  "\x1B[35m"
#define CYAN_COLOR  "\x1B[36m"
#define WHITE_COLOR "\x1B[0m"
#define START_COLOR CYAN_COLOR
#define STOP_COLOR  WHITE_COLOR

#define ADC1_TEST_CHANNEL (6) //6 channel connect to pin34
#define ADC1_TEST_PIN    34 //pin34

#define get_tmr(tm) (xTaskGetTickCount() + tm)
#define check_tmr(tm) (xTaskGetTickCount() >= tm ? true : false)


#pragma once

#pragma pack(push,1)
typedef struct {
    uint32_t num;
    uint8_t type;
} s_evt;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    uint8_t faren;
    float cels;
    uint32_t vcc;
} t_sens_t;
#pragma pack(pop)
extern uint8_t temprature_sens_read();

extern xQueueHandle evtq;

extern uint32_t cli_id;

extern void get_tsensor(t_sens_t *t_s);

extern uint8_t calcx(int len);

extern void printik(const char *tag, const char *buf, const char *color);

#include "ssd1306.h"
#include "serial.h"

