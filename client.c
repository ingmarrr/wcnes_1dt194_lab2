#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/adxl345.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h>

#define SAMPLE_RATE (CLOCK_SECOND / 2)  // 100 Hz sampling
#define MOVEMENT_THRESHOLD 100
#define ALARM_TIMEOUT (10 * CLOCK_SECOND)  // 10 seconds

PROCESS(sensor_process, "Sensor Process");
AUTOSTART_PROCESSES(&sensor_process);

static uint8_t alarm_type = 0;
static int16_t last_x	  = 0;
#define ACCEL_ALARM 1
#define BUTTON_ALARM 2

#define abs(_x) (_x >= 0 ? _x : (-1 * _x))

static int detect_movement(void) {
    int16_t x = adxl345.value(X_AXIS);
    printf("[ABS] %d -> %u\n", x, abs(x));
    bool out = (abs(last_x) - abs(x) > MOVEMENT_THRESHOLD);
    last_x = x;
    return out;
}

PROCESS_THREAD(sensor_process, ev, data) {
    static struct etimer sample_timer;
    static struct etimer alarm_timeout;
    
    PROCESS_BEGIN();
    SENSORS_ACTIVATE(button_sensor);
    nullnet_set_input_callback(NULL);
    
    while(1) {
	printf("[WAITING]\n");
	nullnet_buf = &alarm_type;
	nullnet_len = sizeof(alarm_type);
	NETSTACK_NETWORK.output(NULL);

        etimer_set(&sample_timer, SAMPLE_RATE);
        PROCESS_WAIT_EVENT_UNTIL(
	    etimer_expired(&sample_timer) 
	    || (ev == sensors_event 
	    && data == &button_sensor));
        
        if(ev == sensors_event && data == &button_sensor) {
            if(!(alarm_type & BUTTON_ALARM)) {
                alarm_type |= BUTTON_ALARM;
                etimer_set(&alarm_timeout, ALARM_TIMEOUT);
		printf("[BUTTON] sent\n");
            }
        }
        
        if(detect_movement()) {
            if(!(alarm_type & ACCEL_ALARM)) {
                alarm_type |= ACCEL_ALARM;
                etimer_set(&alarm_timeout, ALARM_TIMEOUT);
		printf("[MOVEMENT] sent\n");
            }
        }
        
        if(etimer_expired(&alarm_timeout)) {
            alarm_type = 0;
	    printf("[ALARM] sent\n");
        }
    }
    
    PROCESS_END();
}
