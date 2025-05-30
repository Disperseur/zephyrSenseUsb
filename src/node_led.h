#ifndef NODE_LED_H
#define NODE_LED_H

#include "global.h"

extern struct k_timer timer_led;

void node_led_init(void);
void _cb_timer_status_led(struct k_timer *tim);

#endif