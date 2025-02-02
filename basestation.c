#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

/* Declare our "main" process, the basestation_process */
PROCESS(basestation_process, "Clicker basestation");
// PROCESS(led_process, "Led Process");
/* The basestation process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process /*, &led_process */);

static struct etimer timer;
static struct etimer inactive_timer;

/* Holds the number of packets received. */

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(
  const void *data, 
  uint16_t len, 
  const linkaddr_t *src, 
  const linkaddr_t *dest
) {
    uint8_t state = *(uint8_t*)data;
    printf("[state]: %u=", state);
    switch (state) {
        case 99: {
            printf("inactive\n");
            if (etimer_expired(&inactive_timer)) 
            {
	            leds_off(LEDS_ALL);
	            return;
            }
        } break;
        case 101: {
            printf("active\n");
	        leds_single_on(0);
            etimer_set(&inactive_timer, CLOCK_SECOND * 10);
        } break;
	case 103: {
	    printf("button-press\n");
	    if (etimer_expired(&inactive_timer))
	    {
		    etimer_set(&inactive_timer, CLOCK_SECOND * 10);
		    leds_single_on(1);
		    return;
	    }
	    leds_on(LEDS_ALL);
	} break;
        default: return;
    }
}

/* Our main process. */
PROCESS_THREAD(basestation_process, ev, data) {
    PROCESS_BEGIN();
    nullnet_set_input_callback(recv);

    while (1) {
	    etimer_set(&timer, CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
	    if (etimer_expired(&inactive_timer)) leds_off(LEDS_ALL);
    }

    PROCESS_END();
}
