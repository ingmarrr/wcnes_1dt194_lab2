#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "dev/adxl345.h"

#define ACCM_READ_INTERVAL    CLOCK_SECOND

PROCESS(client_process, "Client Process");
PROCESS(inactive_process, "Led Process");
PROCESS(button_process, "Button Process");
AUTOSTART_PROCESSES(&client_process, &inactive_process, &button_process);

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
}

static struct etimer timer;
static uint8_t payload = 99;

void accm_tap_cb(uint8_t reg) {
    payload = 101;
    printf("[%u] Tap detected!\n", ((uint16_t) clock_time())/128);
	process_poll(&client_process);
}

PROCESS_THREAD(inactive_process, ev, data) {
    PROCESS_BEGIN();
    while(1) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        leds_on(LEDS_GREEN);
        printf("inactive\n");
        payload = 99;
    }
    PROCESS_END();
}

PROCESS_THREAD(button_process, ev, data) {
    PROCESS_BEGIN();
    SENSORS_ACTIVATE(button_sensor);
	process_poll(&client_process);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);
        if (data == &button_sensor)
        {
            payload = 103;
        }
	
    }
    PROCESS_END();
}

/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
    PROCESS_BEGIN();

    /* Initialize NullNet */
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);
    nullnet_set_input_callback(recv);

    accm_init();
    ACCM_REGISTER_INT1_CB(accm_tap_cb);
    ACCM_REGISTER_INT2_CB(accm_tap_cb);
    accm_set_irq(ADXL345_INT_TAP, ADXL345_INT_TAP);

    while (1) {
	    printf("sending\n");
        leds_toggle(LEDS_RED);

	    memcpy(nullnet_buf, &payload, sizeof(payload));
	    nullnet_len = sizeof(payload);

	    NETSTACK_NETWORK.output(NULL);

	    etimer_set(&timer, ACCM_READ_INTERVAL);
	    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		printf("deactivating\n");
        process_poll(&inactive_process);
    }
    PROCESS_END();
}