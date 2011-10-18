#include "b91.h"


enum {
	T_GPS_VALID = 0x20,

	T_NMEA_OTHER = 0x00,
	T_NMEA_GLL = 0x08,
	T_NMEA_GGA = 0x10,
	T_NMEA_RMC = 0x18,

	T_COMP_COMP = 0x00,
};

#define B91_SIZE 4
uint32_t b91_enc[] = { 753571, 8281, 91, 1}; 

/* stolen from opentracker */
const unsigned far int b91_speed[] = {
	0, 2, 4, 7, 9, 12, 15, 18, 22, 26, 30, 34, 39, 44,
	50, 56, 62, 69, 77, 85, 94, 104, 114, 125, 137, 150,
	165, 180, 196, 214, 233, 254, 276, 300, 326, 355, 385,
	418, 453, 492, 533, 578, 626, 678, 735, 795, 861, 932,
	1009, 1091, 1181, 1277, 1381, 1494, 1616, 1747, 1889,
	2042, 2207, 2386, 2579, 2787, 3012, 3255, 3518, 3801,
	4107, 4438, 4795, 5181, 5597, 6047, 6533, 7058, 7624,
	8237, 8897, 9611, 10382, 11215, 12114, 13085, 14134,
	15267, 16491, 17812, 19239, 20780, 22444, 24242, 65535
};

void b91_encode(uint8_t *buf, uint32_t in)
{
	int i;
	uint32_t out;

	for (i = 0; i < B91_SIZE; i++) {
		out = in / b91_enc[i];
		in = in % b91_enc[i];

		buf[i] = '!' + (out & 0xff);
	}
}

/* TODO: rewrite without floats */
void b91_encode_lat(uint8_t *buf, uint32_t lat, uint8_t lat_h)
{
	uint32_t ref = 900000; // 90deg
	uint32_t min;
	uint32_t tmp;
	double deg;

	// 2447.0949 ddmm.mmmmm => 24470949
	// min = 470949
	min = lat % 1000000;

	// 24470949 - 470949 = 24000000
	tmp = lat - min;

	// mmmmmm => dddddd
	// 7849
	min /= 60;

	// 24000000/100 + 7849 = 247849 => dd.dddd
	tmp = min + tmp / 100;

	if (lat_h == 'S') {
		ref += tmp;
	} else {
		ref -= tmp;
	}

	deg = ref / 10000;

	b91_encode(buf, 380926 * deg);
}

void b91_encode_lon(uint8_t *buf, uint32_t lon, uint8_t lon_h)
{
	uint32_t ref = 1800000; // 90deg
	uint32_t min;
	uint32_t tmp;
	double deg;

	// 2447.0949 ddmm.mmmmm => 24470949
	// min = 470949
	min = lon % 1000000;

	// 24470949 - 470949 = 24000000
	tmp = lon - min;

	// mmmmmm => dddddd
	// 7849
	min /= 60;

	// 24000000/100 + 7849 = 247849 => dd.dddd
	tmp = min + tmp / 100;

	if (lon_h == 'W') {
		ref -= tmp;
	} else {
		ref += tmp;
	}

	deg = ref / 10000;

	b91_encode(buf, 190463 * deg);
}

void b91_encode_course(uint8_t *buf, uint16_t course)
{
	course >>= 2;
	buf[0] = '!' + (course & 0xff);
}

void b91_encode_speed(uint8_t *buf, uint16_t speed)
{
	uint8_t c = 0;

	while (b91_speed[c] < (speed & 0x7fff))
		c++;
	
	buf[0] = '!' + c;
}

void b91_encode_type(uint8_t *buf, bool valid)
{
	uint8_t t = 0;

	t |= T_NMEA_RMC | T_COMP_COMP;

	if (valid) {
		t |= T_GPS_VALID;
	}

	buf[0] = '!' + c;
}
