
#include <drv/timer.h>
#include "sb.h"

#define LOG_LEVEL  3
#define LOG_FORMAT LOG_FMT_VERBOSE
#include <cfg/log.h>


uint16_t sb_rate;
uint16_t sb_last_heading;
ticks_t sb_last_send; 

#define SEC_SINCE_BEACON (ticks_to_ms(timer_clock() - sb_last_send)/1000)

// todo: make configurable
struct sb_config sb_cfg;

void sb_init()
{
	sb_cfg.enabled = true,
	sb_cfg.high_speed = 96, // km/h
	sb_cfg.high_rate = 40, // sec
	sb_cfg.low_speed = 8,   // km/h
	sb_cfg.low_rate = 1800, // sec
	sb_cfg.min_turn_time = 15, // sec
	sb_cfg.min_turn_angle = 30,
	sb_cfg.turn_slope = 255,

	sb_last_send = timer_clock();
	sb_last_heading = 999;
	sb_rate = 120;
}

void sb_calculate(uint16_t speed, uint16_t course, bool *beacon_now)
{
	if (sb_cfg.enabled == false)
		return;

	if (UNLIKELY(sb_last_heading == 999)) {
		sb_last_heading = course;
	}

	if (speed < sb_cfg.low_speed) {
		sb_rate = sb_cfg.low_rate;
	} else {
		if (speed > sb_cfg.high_speed) {
			sb_rate = sb_cfg.high_rate;
		} else {
			sb_rate = sb_cfg.high_rate * sb_cfg.high_speed / speed;
		}

		uint16_t turn_threshold = sb_cfg.min_turn_angle + sb_cfg.turn_slope / speed;
		int32_t diff = sb_last_heading - course;

		if (diff > 180) {
			diff -= 360;
		} else if (diff < 180) {
			diff += 360;
		}

		LOG_INFO("tt %d last_course %d course %d diff %ld\n", turn_threshold, sb_last_heading, course, diff);
		if ((ABS(diff) > turn_threshold) && (SEC_SINCE_BEACON > sb_cfg.min_turn_time)) {
			sb_last_heading = course; 
			*beacon_now = true;	
		}
	}

	LOG_INFO("speed %d\n", speed);
}

bool sb_send(bool beacon_now)
{
	if (beacon_now || (SEC_SINCE_BEACON > sb_rate)) { 
		LOG_INFO("since %ld sb_rate %d\n", SEC_SINCE_BEACON, sb_rate);
		sb_last_send = timer_clock();
		return true;
	}

	return false;
}

