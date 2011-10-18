#include <string.h>

#include <cfg/debug.h>
#define LOG_LEVEL  3
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#include "nmea.h"

nmea_parser_t nmea_parser;
struct gpsdata_t gpsdata;

#define NMEA_GPSSIZE 128
uint8_t nmea_GpsBuf[NMEA_GPSSIZE];
uint8_t nmea_GpsIdx = 0;

void nmea_init(nmea_parser_t parser)
{
	nmea_parser = parser;
}

static void nmea_push(uint8_t ch)
{
	if ((nmea_GpsIdx == NMEA_GPSSIZE-1) || (ch == '$')) {
		nmea_GpsIdx = 0;
	}

	nmea_GpsBuf[nmea_GpsIdx] = ch;
	nmea_GpsIdx++;

	if ((nmea_GpsIdx > 6) && (nmea_GpsBuf[0] == '$') && (ch == '\n')) {
		if (nmea_parser)
			nmea_parser(nmea_GpsBuf, nmea_GpsIdx);

		nmea_GpsIdx = 0;
		// remove $ sign
		nmea_GpsBuf[0] = ' ';
	}
}

void nmea_poll(KFile *channel)
{
	int ch, e;
	while ((ch = kfile_getc(channel)) != EOF) {
		nmea_push(ch);
	}

	if ((e = kfile_error(channel))) {
		if (e & ~4) {
			LOG_ERR("ch error [%0X]\n", e);
		}
		kfile_clearerr(channel);
	}
}

static uint32_t nmea_atoi(char *p)
{
	uint32_t out = 0;

	while ((*p >= '0' && *p <= '9') || *p == '.')
	{
		if (*p == '.') {
			p++;
			continue;
		}
		out *= 10;
		out += *p - '0';
		p++;
	}

	return out;
}

void nmea_parse(uint8_t *buf, uint8_t len)
{
	(void)len;

	char *p;

	p = (char *)buf;
/*
	if(!strncmp(p, "$GPGGA", 6))
	{	
		p += 7;

		gpsdata.hour = (p[0] - '0') * 10 + p[1] - '0';
		gpsdata.min = (p[2] - '0') * 10 + p[3] - '0';
		gpsdata.sec = (p[4] - '0') * 10 + p[5] - '0';

//		if (p[6] == '.')
//			gpsdata.hsec = (p[7] - '0') * 10 + p[8] - '0';
//		else
//			gpsdata.hsec = 0;
//
		p = strstr(p, ",") + 1;

		// parse lat
		tmp = nmea_atoi(p);
		p = strstr(p, ",") + 1;
		gpsdata.lat_h = p[0];
		gpsdata.lat = tmp;

		p = strstr(p, ",")+1;
		// parse lon
		tmp = nmea_atoi(p);
		p=strstr(p, ",") + 1;
		gpsdata.lon_h = p[0];
		gpsdata.lon = tmp;

		p = strstr(p, ",") + 1;
		gpsdata.valid = (p[0] != '0')?1:0;

		p = strstr(p, ",") + 1;
		gpsdata.sats = (p[0] - '0') * 10 + p[1] - '0';

		p = strstr(p, ",") + 1;
		gpsdata.hdop = nmea_atoi(p);

		p = strstr(p, ",") + 1;
		gpsdata.alt = nmea_atoi(p);

	} else*/
	if(!strncmp(p, "$GPRMC", 6)) {	
		p += 7;
		gpsdata.hour = (p[0] - '0') * 10 + p[1] - '0';
		gpsdata.min = (p[2] - '0') * 10 + p[3] - '0';
		gpsdata.sec = (p[4] - '0') * 10 + p[5] - '0';
		
//		if(p[6] == '.')
//			gpsdata.hsec = (p[7]-'0')*10 + p[8]-'0';
//		else
//			gpsdata.hsec=0;

		p = strstr(p, ",") + 1;
		gpsdata.valid = (p[0] == 'A')?1:0;

		// parse lat
		p = strstr(p, ",") + 1;
		gpsdata.lat = nmea_atoi(p);

		p = strstr(p, ",") + 1;
		gpsdata.lat_h = p[0];

		// parse lon
		p = strstr(p, ",")+1;
		gpsdata.lon = nmea_atoi(p);

		p=strstr(p, ",") + 1;
		gpsdata.lon_h = p[0];

		p = strstr(p, ",") + 1;
		gpsdata.speed = nmea_atoi(p);

		p = strstr(p, ",") + 1;
		gpsdata.heading = nmea_atoi(p);

		p = strstr(p, ",") + 1;
		gpsdata.day = (p[0] - '0') * 10 + p[1] - '0';
		gpsdata.month = (p[2] - '0') * 10 + p[3] - '0';
		gpsdata.year = (p[4] - '0') * 10 + p[5] - '0';

		// gprmc is last message in burst
		gpsdata.updated = 1;
	}

}
