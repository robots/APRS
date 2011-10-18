/* \brief Empty project.
 *
 * This is a minimalist project, it just initializes the hardware of the
 * supported board and proposes an empty main.
 */

#include <string.h>
#include "hw/hw_led.h"

#include <cfg/debug.h>

#include <cpu/irq.h>

#include <drv/timer.h>

#include <net/afsk.h>
#include <drv/ser.h>
#include <net/kiss.h>

#define LOG_LEVEL  3
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#include "ad.h"
#include "da.h"

Afsk afsk;
AX25Ctx ax25;
static struct Serial ser_port;

static void ax25_message_callback(struct AX25Msg *msg)
{
	(void) msg;
	kiss_send_host(0x00, ax25.buf, ax25.frm_len - 2);
}

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

	ser_init(&ser_port, SER_UART1);
	ser_setbaudrate(&ser_port, 115200L);

	afsk_init(&afsk, 0, 0);

	// timer period = 24000000 hz /100/25 = 9600hz
	AD_Init(&afsk);
	AD_SetTimer(100, 25);
	AD_Start();

	DA_Init(&afsk);
	DA_SetTimer(100, 25);

	kiss_init(&ser_port, &ax25, &afsk);

	ax25_init(&ax25, &afsk.fd, 1, ax25_message_callback);
}

int main(void)
{
	init();

	LOG_INFO("\r\n== BeRTOS TNC\r\n");
	LOG_INFO("== Starting.\r\n");
	kfile_printf(&ser_port.fd, "\r\n== BeRTOS TNC\r\n");
	kfile_printf(&ser_port.fd, "== Starting.\r\n" );

	while (1) {
		ax25_poll(&ax25);

		if (ax25.dcd) {
			LED_BLUE_ON();
		} else {
			LED_BLUE_OFF();
		}

		if (afsk.sending) {
			LED_GREEN_ON();
		} else {
			LED_GREEN_OFF();
		}
		
		kiss_serial_poll();
		kiss_queue_process();
	}

	return 0;
}

