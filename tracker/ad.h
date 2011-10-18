#ifndef AD_H_
#define AD_H_

#include <net/afsk.h>

void AD_Init(Afsk *);
void AD_Start(void);
void AD_Stop(void);
void AD_SetTimer(uint16_t, uint16_t);

#endif

