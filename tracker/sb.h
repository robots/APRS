#ifndef SB_h_
#define SB_h_

struct sb_config {
	bool enabled;

	uint16_t high_speed;
	ticks_t high_rate;

	uint16_t low_speed;
	ticks_t low_rate;

	ticks_t min_turn_time;
	uint16_t min_turn_angle;
	uint16_t turn_slope;
};

/* interval in seconds */
extern uint16_t sb_rate;

void sb_init(void);
void sb_calculate(uint16_t speed, uint16_t course, bool *beacon_now);
bool sb_send(bool beacon_now);

#endif

