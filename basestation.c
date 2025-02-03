#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h>

#define ALARM_TIMEOUT (10 * CLOCK_SECOND)

PROCESS(base_station_process, "Base Station Process");
PROCESS(led_process, "LED Process");
AUTOSTART_PROCESSES(&base_station_process, &led_process);

static uint8_t current_alarm = 0;

// Callback function for received packets
static void receive_callback(
    const void *data, 
    uint16_t len,
    const linkaddr_t *src, 
    const linkaddr_t *dest
) {
    if(len == sizeof(uint8_t)) {
        current_alarm = *(uint8_t *)data;
	printf("[MESSAGE] %u\r\n", current_alarm);
    }
    process_poll(&led_process);
}

PROCESS_THREAD(base_station_process, ev, data) {
    PROCESS_BEGIN();
    nullnet_set_input_callback(receive_callback);
    while(1) PROCESS_WAIT_EVENT();
    PROCESS_END();
}

PROCESS_THREAD(led_process, ev, data) {
    static struct etimer led_timer;
    uint8_t current_alarm_copy = current_alarm;
    PROCESS_BEGIN();
    
    while(1) 
    {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
	printf("  [CURRENT] %u\r\n", current_alarm);
        
	switch (current_alarm_copy) {
	    case 1: {
		printf("[MOVEMENT]\r\n");
		leds_single_on(0);
	    } break;
	    case 2: {
		printf("[BUTTON]\r\n");
		leds_single_on(1);
	    } break;
	    case 3: {
		printf("[BOTH]\r\n");
		leds_on(LEDS_ALL);
	    } break;
	    default: {
		etimer_set(&led_timer, ALARM_TIMEOUT);
	    } break;
	}
        
        if (current_alarm_copy == 0) 
	{
	    printf("[DEACTIVATE]\r\n");
            leds_off(LEDS_ALL);
        }
    }
    
    PROCESS_END();
}
