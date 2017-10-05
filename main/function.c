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
uint8_t calcx(int len)
{
uint8_t ret = 0;

    if ( (len > 0) && (len < 16) ) ret = ((16 - len) >> 1) + 1;

    return ret;
}
//--------------------------------------------------------------------



