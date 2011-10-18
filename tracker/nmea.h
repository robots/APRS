#ifndef NMEA_h_
#define NMEA_h_

#include <stdint.h>
#include <io/kfile.h>

typedef void (*nmea_parser_t)(uint8_t *, uint8_t);

struct gpsdata_t {
	// time
	uint8_t hour;
	uint8_t min;
	uint8_t sec;

	// date
	uint8_t day;
	uint8_t month;
	uint8_t year;

	// position
	uint8_t valid;
	uint32_t lat;
	uint8_t lat_h;

	uint32_t lon;
	uint8_t lon_h;

	uint16_t alt;

	uint16_t heading;
	uint16_t speed;

	uint8_t sats;
	uint8_t hdop;

	uint8_t updated;
};


void nmea_init(nmea_parser_t parser);
void nmea_poll(KFile *channel);
void nmea_parse(uint8_t *buf, uint8_t len);

#endif

