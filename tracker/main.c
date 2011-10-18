/* \brief Empty project.
 *
 * This is a minimalist project, it just initializes the hardware of the
 * supported board and proposes an empty main.
 */

#include <string.h>
#include <stdlib.h>
#include "hw/hw_led.h"

#include <cfg/debug.h>

#include <cpu/irq.h>

#include <drv/timer.h>

#include <net/afsk.h>
#include <net/ax25.h>
#include <net/kiss.h>
#include <drv/ser.h>

#define LOG_LEVEL  3
#define LOG_FORMAT LOG_FMT_VERBOSE
#include <cfg/log.h>

#include "b91.h"
#include "ad.h"
#include "da.h"
#include "sb.h"
#include "nmea.h"

#include "stm32f10x_conf.h"

#define DEBOUNCE_TIMER  50UL
#define DEBOUNCE_CNT    20

#define KNOT2KPH(x) (((x)*1852)/1000)

#define GPIO_BASE_A  ((struct stm32_gpio *)GPIOA_BASE)
#define GPIO_BASE_B  ((struct stm32_gpio *)GPIOB_BASE)

#define GPSEN_PIN    BV(11)
#define FLASHCS_PIN    BV(7)
#define RFMCS_PIN    BV(7)
#define RFMRST_PIN    BV(1)

#define TDO_PIN    BV(3)
#define TRST_PIN    BV(4)

Afsk afsk;
AX25Ctx ax25;
static struct Serial ser_port;
static struct Serial gps_port;
extern struct gpsdata_t gpsdata;

static AX25Call path[] = AX25_PATH(AX25_CALL("apzbrt", 0), AX25_CALL("om5amx", 9), AX25_CALL("wide1", 1), AX25_CALL("wide2", 2));
static uint8_t aprs_msg_len;
static char aprs_msg[255];

static void init(void)
{
	/* Enable all the interrupts */
	IRQ_ENABLE;

	/* Initialize debugging module (allow kprintf(), etc.) */
	kdbg_init();

	/* Initialize system timer */
	timer_init();

	/* Initialize LED driver */
	LED_INIT();

	RCC->APB2ENR |= RCC_APB2_GPIOA;
	RCC->APB2ENR |= RCC_APB2_GPIOB;

	// disable jtag, only swd
	AFIO->MAPR |= 0x02000000; 

	stm32_gpioPinConfig(GPIO_BASE_B, TRST_PIN, GPIO_MODE_IPU, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(GPIO_BASE_B, TDO_PIN, GPIO_MODE_IPU, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(GPIO_BASE_A, GPSEN_PIN | RFMCS_PIN | RFMRST_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(GPIO_BASE_B, FLASHCS_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);

	// disable GPS
	stm32_gpioPinWrite(GPIO_BASE_A, GPSEN_PIN, 0);

	// disable flash CS, RFM cs, and enable RFM rst
	stm32_gpioPinWrite(GPIO_BASE_B, FLASHCS_PIN, 1);
	stm32_gpioPinWrite(GPIO_BASE_A, RFMCS_PIN, 1);
	stm32_gpioPinWrite(GPIO_BASE_A, RFMRST_PIN, 0);


	ser_init(&gps_port, SER_UART1);
	ser_setbaudrate(&gps_port, 9200L);
	ser_settimeouts(&gps_port, 0, 0);

	ser_init(&ser_port, SER_UART2);
	ser_setbaudrate(&ser_port, 115200L);

	kiss_init(&ser_port, &ax25, &afsk);

	afsk_init(&afsk, 0, 0);

	// timer period = 24000000 hz /100/25 = 9600hz
	AD_Init(&afsk);
	AD_SetTimer(100, 25);
	AD_Start();

	DA_Init(&afsk);
	DA_SetTimer(100, 25);

}

static void ax25_message_callback(struct AX25Msg *msg)
{
	(void) msg;
	kiss_send_host(0x00, ax25.buf, ax25.frm_len - 2);
}

static void nmea_callback(uint8_t *buf, uint8_t len)
{
	kiss_send_host(0x01, buf, len);
}

static void message_callback(struct AX25Msg *msg)
{
	kfile_printf(&ser_port.fd, "\n\nSRC[%.6s-%d], DST[%.6s-%d]\r\n", msg->src.call, msg->src.ssid, msg->dst.call, msg->dst.ssid);

	for (int i = 0; i < msg->rpt_cnt; i++)
		kfile_printf(&ser_port.fd, "via: [%.6s-%d]\r\n", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);

	kfile_printf(&ser_port.fd, "DATA: %.*s\r\n", msg->len, msg->info);
}

static void tracker()
{
	ticks_t debounce = timer_clock();
	uint16_t debounce_cnt = 0;
	bool button = false;

	bool beacon_now = true;

	kfile_printf( &ser_port.fd, "\r\n== Tracker Mode ==\r\n" ) ;

	sb_init();

	nmea_init(nmea_parse);

	stm32_gpioPinWrite(GPIO_BASE_A, GPSEN_PIN, 1);

	ax25_init(&ax25, &afsk.fd, 0, message_callback);

	while (1) {
		nmea_poll(&gps_port.fd);

		ax25_poll(&ax25);

		if (ax25.dcd) {
			LED_YEL_ON();
		} else {
			LED_YEL_OFF();
		}

		if (afsk.sending) {
			LED_GRN_ON();
		} else {
			LED_GRN_OFF();
		}

		// is button pressed ? 
		if (timer_clock() - debounce > ms_to_ticks(DEBOUNCE_TIMER)) {
			debounce = timer_clock();

			if (!stm32_gpioPinRead(GPIO_BASE_B, TDO_PIN)) {
				if (debounce_cnt <= DEBOUNCE_CNT) {
					debounce_cnt++;
				}
			} else {
				if (debounce_cnt > 0) {
					debounce_cnt--;
				}
			}

			if (debounce_cnt >= DEBOUNCE_CNT && button == false) {
				LOG_INFO("Button pressed\n");
				button = true;
				beacon_now = true;
			} else if (debounce_cnt == 0 && button == true) {
				LOG_INFO("Button released\n");
				button = false;
			}
		}

		if (gpsdata.updated == 1) {
			gpsdata.updated = 0;

			sb_calculate(KNOT2KPH(gpsdata.speed/10), gpsdata.heading/10, &beacon_now);
/*
			aprs_msg_len = sprintf((char*)aprs_msg, "!%02d%02d%02dh%02d%02d.%02d%c/%03d%02d.%02d%c%c%03d/%03d%s",
				gpsdata.hour, gpsdata.min, gpsdata.sec,
				(int)((gpsdata.lat/1000000)%100), (int)((gpsdata.lat/10000)%100), (int)((gpsdata.lat/100) % 100), gpsdata.lat_h,
				(int)((gpsdata.lon/1000000)%100), (int)((gpsdata.lon/10000)%100), (int)((gpsdata.lon/100) % 100), gpsdata.lon_h,
				'>', // car icon
				(int)(gpsdata.heading/10),
				(int)(gpsdata.speed/10),
				"Mike on road!" // comment
				);
*/			
/*
 * TODO: compressed position, csT missing
			aprs[0] = '/'; // symbol table
			b91_encode_lat(aprs_msg+1, gpsdata.lat, gpsdata.lat_h);
			b91_encode_lon(aprs_msg+5, gpsdata.lon, gpsdata.lon_h);
			aprs_msg[10] = '>'; // car symbol
			b91_encode_course(aprs_msg+11, gpsdata.course/10);
			b91_encode_speed(aprs_msg+12, gpsdata.speed/10);
			b91_encode_type(aprs_msg+13, gpsdata.valid);
			aprs_msg[14] = '\0';
			LOG_INFO("Message compressed %s\n", aprs_msg);
*/
			aprs_msg_len = sprintf(aprs_msg, "%c%02d%02d.%02d%c%c%03d%02d.%02d%c%c%03d/%03d%s",
				'!', // realtime APRS with no messaging
				(int)((gpsdata.lat/1000000)%100), (int)((gpsdata.lat/10000)%100), (int)((gpsdata.lat/100) % 100), gpsdata.lat_h,
				'/', // primary symbol table
				(int)((gpsdata.lon/1000000)%100), (int)((gpsdata.lon/10000)%100), (int)((gpsdata.lon/100) % 100), gpsdata.lon_h,
				'>', // car icon
				(int)(gpsdata.heading/10),
				(int)(gpsdata.speed/10),
				"Mike on road!" // comment
				);
			aprs_msg[aprs_msg_len] = 0;

			LOG_INFO("Message %s\n", aprs_msg);
		}

		if ((gpsdata.valid == 1) && (ax25.dcd == false) /*&& (afsk.cd == false) && (timer_clock() - start > ms_to_ticks(10000L))*/)
		{
			//start = timer_clock();
//			if (beacon_now) {
//				LOG_INFO("Beacon now !\n");
//			}

			if (sb_send(beacon_now)) {
				beacon_now = false;
				LOG_INFO("Transmitting !\n");
				ax25_sendVia(&ax25, path, countof(path), aprs_msg, aprs_msg_len);
			}
		}
	}
}

static void tnc()
{

	kfile_printf(&ser_port.fd, "\r\n== TNC Mode ==\r\n");

	nmea_init(nmea_callback);
	stm32_gpioPinWrite(GPIO_BASE_A, GPSEN_PIN, 1);

	ax25_init(&ax25, &afsk.fd, 1, ax25_message_callback);

	while (1) {
		nmea_poll(&gps_port.fd);
		ax25_poll(&ax25);

		if (ax25.dcd) {
			LED_YEL_ON();
		} else {
			LED_YEL_OFF();
		}

		if (afsk.sending) {
			LED_GRN_ON();
		} else {
			LED_GRN_OFF();
		}
		
		kiss_serial_poll();
		kiss_queue_process();
	}

}

int main(void)
{
	init();

	kfile_printf(&ser_port.fd, "== Starting.\r\n");

	bool mode_button;

	mode_button = stm32_gpioPinRead(GPIO_BASE_B, TRST_PIN);

	if (mode_button) {
		tracker();
	}

	tnc();

	return 0;
}

