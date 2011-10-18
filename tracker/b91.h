#ifndef B91_h
#define B91_h

#include <stdint.h>

void b91_encode(uint8_t *buf, uint32_t in);
void b91_encode_lat(uint8_t *buf, uint32_t lat, uint8_t lat_h);
void b91_encode_lon(uint8_t *buf, uint32_t lon, uint8_t lon_h);
void b91_encode_course(uint8_t *buf, uint16_t course);
void b91_encode_speed(uint8_t *buf, uint16_t speed);

#endif
