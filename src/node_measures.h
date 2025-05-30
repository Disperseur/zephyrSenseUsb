#ifndef NODE_MEASURES_H
#define NODE_MEASURES_H

#include "global.h"

extern struct k_thread measures_thread_data;
extern struct k_sem sem_measures;
extern struct k_timer timer_measures;

void node_measures_init(void);
void _handler_measures(void*, void*, void*);
void _cb_timer_measures(struct k_timer *tim);
void setup_sensor_acceleration(const struct device *sensor_acceleration);

#endif