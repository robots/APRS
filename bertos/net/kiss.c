#include <stdint.h>
#include <string.h>

#include <algo/rand.h>

#include <net/kiss.h>

Serial *kiss_ser;
AX25Ctx *kiss_ax25;
Afsk *kiss_afsk;

ticks_t kiss_queue_ts;
uint8_t kiss_queue_state;
uint8_t kiss_queue_len = 0;
struct Kiss_msg kiss_queue[KISS_QUEUE];

uint8_t kiss_txdelay;
uint8_t kiss_txtail;
uint8_t kiss_persistence;
uint8_t kiss_slot_time;
uint8_t kiss_duplex;

static void kiss_cmd_process(struct Kiss_msg *k);

void kiss_init(Serial *ser, AX25Ctx *ax25, Afsk *afsk)
{
	kiss_txdelay = 50;
	kiss_persistence = 63;
	kiss_txtail = 5;
	kiss_slot_time = 10;
	kiss_duplex = KISS_DUPLEX_HALF;

	kiss_ser = ser;
	kiss_ax25 = ax25;
	kiss_afsk = afsk;
}

void kiss_serial_poll()
{
	static struct Kiss_msg k = {.pos = 0};
	static bool escaped = false;
	int c;

	c = ser_getchar_nowait(kiss_ser);

	if (c == EOF) {
		return;
	}
	
	// sanity checks
	// no serial input in last 2 secs?
	if (timer_clock() - k.last_tick  >  ms_to_ticks(2000L)) {
		k.pos = 0;
	}
	
	// about to overflow buffer? reset
	if (k.pos >= ((uint8_t)CONFIG_AX25_FRAME_BUF_LEN - 2)) {
		k.pos = 0;
	}

	if (c == KISS_FEND) {
		if ((!escaped) && (k.pos > 0)) {
			kiss_cmd_process(&k);
		}

		k.pos = 0;
		escaped = false;
		return;
	} else if (c == KISS_FESC) {
		escaped = true;
		return;
	} else if (c == KISS_TFESC) {
		if (escaped) {
			escaped = false;
			c = KISS_FESC;
		}
	} else if (c == KISS_FEND) {
		if (escaped) {
			escaped = false;
			c = KISS_FEND;
		}
	} else if (escaped) {
		escaped = false;
	}
	
	k.buf[k.pos] = c & 0xff;
	k.pos++;
	k.last_tick = timer_clock();
}

static void kiss_cmd_process(struct Kiss_msg *k)
{
	uint8_t cmd;
	uint8_t port;

	cmd = k->buf[0] & 0x0f;
	port = k->buf[0] >> 4;

	if (port != 0)
		return;

	if (cmd == KISS_CMD_DATA) {
		kiss_queue_message(k->buf + 1, k->pos - 1);
		//ax25_sendRaw(kiss_ax25, k->buf+1, k->pos-1);
		return;
	}

	if (k->pos < 2) {
		return;
	}
	
	if (cmd == KISS_CMD_TXDELAY) {
		kiss_txdelay = k->buf[1];
	} else if (cmd == KISS_CMD_P) {
		kiss_persistence = k->buf[1];
	} else if (cmd == KISS_CMD_SlotTime) {
		kiss_slot_time = k->buf[1];
	} else if (cmd == KISS_CMD_TXtail) {
		kiss_txtail = k->buf[1];
	} else if (cmd == KISS_CMD_FullDuplex) {
		kiss_duplex = k->buf[1];
	}
}

void kiss_queue_message(uint8_t *buf, size_t len)
{
	if (kiss_queue_len == KISS_QUEUE)
		return;

	memcpy(kiss_queue[kiss_queue_len].buf, buf, len);
	kiss_queue[kiss_queue_len].pos = len;
	kiss_queue_len ++;
}

void kiss_queue_process()
{
	uint8_t random;

	if (kiss_queue_len == 0) {
		return;
	}

	if (kiss_ax25->dcd) {
		return;
	}

	random = (uint32_t)rand() & 0xff;

	if (kiss_queue_state == KISS_QUEUE_DELAYED) {
		if (timer_clock() - kiss_queue_ts <= ms_to_ticks(kiss_slot_time * 10)) {
			return;
		}
	} else if (random > kiss_persistence) {

		kiss_queue_state = KISS_QUEUE_DELAYED;
		kiss_queue_ts = timer_clock();

		return;
	}

	for (uint8_t i = 0; i < kiss_queue_len; i++) {
		ax25_sendRaw(kiss_ax25, kiss_queue[i].buf, kiss_queue[i].pos);
	}

	kiss_queue_len = 0;
	kiss_queue_state = KISS_QUEUE_IDLE;
}

void kiss_send_host(uint8_t ch, uint8_t *buf, uint8_t len)
{
	uint8_t i;

	kfile_putc(KISS_FEND, &kiss_ser->fd);
	kfile_putc((ch << 4) & 0xf0, &kiss_ser->fd);

	for (i = 0; i < len; i++) {

		uint8_t c = buf[i];

		if (c == KISS_FEND) {
			kfile_putc(KISS_FESC, &kiss_ser->fd);
			kfile_putc(KISS_TFEND, &kiss_ser->fd);
			continue;
		}

		kfile_putc(c, &kiss_ser->fd);

		if (c == KISS_FESC) {
			kfile_putc(KISS_TFESC, &kiss_ser->fd);
		}
	}

	kfile_putc(KISS_FEND, &kiss_ser->fd);
}

