#ifndef DA_H_
#define DA_H_

#include <net/afsk.h>

void DA_Init(Afsk *afsk);
void DA_SetTimer(uint16_t prescaler, uint16_t period);
void DA_Start(void);
void DA_Stop(void);

#endif
