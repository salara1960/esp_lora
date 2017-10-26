#include "function.h"


uint32_t cli_id = 0;
xQueueHandle evtq = NULL;

//--------------------------------------------------------------------

inline void get_tsensor(t_sens_t *t_s)
{
    t_s->vcc = (uint32_t)(adc1_get_raw(ADC1_TEST_CHANNEL) * 0.8);
    t_s->faren = temprature_sens_read();// - 40;
    t_s->cels = (t_s->faren - 32) * 5/9;
}
//--------------------------------------------------------------------

void printik(const char *tag, const char *buf, const char *color)
{
    int len = strlen(tag) + strlen(buf) + strlen(color) + 64;
    char *st = (char *)calloc(1, len);
    if (st) {
	struct timeval tvl;
	gettimeofday(&tvl, NULL);
	time_t it_ct = tvl.tv_sec;
	struct tm *ctimka = localtime(&it_ct);
	sprintf(st,"%02d:%02d:%02d.%03d | ", ctimka->tm_hour, ctimka->tm_min, ctimka->tm_sec, (int)(tvl.tv_usec/1000));
	sprintf(st+strlen(st),"%s[%s]", color, tag);
	sprintf(st+strlen(st)," : %s%s", buf, STOP_COLOR);
	if (!strchr(st,'\n')) sprintf(st+strlen(st),"\n");
	//if (st[strlen(st)-1] != '\n') sprintf(st+strlen(st),"\n");
	printf(st);
	free(st);
    }

}
//--------------------------------------------------------------------
