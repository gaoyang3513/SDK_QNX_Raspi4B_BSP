#ifndef __BSP_TH_H__
#define __BSP_TH_H__    

#include <stdint.h>
#include <devctl.h>

enum th_pwr {
    TH_PWR_OFF = 0,
    TH_PWR_ON,
    TH_PWR_RESET,
    TH_PWR_MAX
};

struct th_data {
    uint32_t temperature;
    uint32_t humidity;
};

#define DCMD_TH_SET_PWR     __DIOT (_DCMD_MIXER, 0x00, enum th_pwr)
#define DCMD_TH_GET_DATA    __DIOTF(_DCMD_MIXER, 0x01, struct th_data)

#endif /* __BSP_TH_H__ */