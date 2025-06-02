#ifndef GLOBAL_H
#define GLOBAL_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>

#define LED_RED    DT_ALIAS(led0)
#define LED_GREEN  DT_ALIAS(led1)
#define LED_BLUE   DT_ALIAS(led2)
#define COMMAND_BUFFER_SIZE 100
#define RINGBUFFER_SIZE 10

typedef enum {  RUNNING,
                STOPPED,
                FAULT} _board_status_t;

typedef enum {  STREAMING,
                ONESHOT,
                RINGBUFFER} _board_mode_t;

typedef enum {  NONE,
                TEMPERATURE,
                PRESSURE,
                HUMIDITY,
                ALTITUDE,
                ACCELERATION,
                GYROSCOPE,
                MAGFIELD, 
                ALL,
                ENV,
                MOTION
              } _sensor_type_t;


typedef struct _accel_t {
    int64_t ax, ay, az;
} accel_t;

typedef struct _gyro_t {
    int64_t gx, gy, gz;
} gyro_t;

typedef struct _data_t {
    int64_t temperature, humidity, pressure, altitude;
    accel_t accel;
    gyro_t  gyro;
} data_t;

typedef struct _rgbled_t {
    struct gpio_dt_spec led_red, led_green, led_blue;
} rgbled_t;

typedef struct _devices_t {
    const struct device *sensor_pressure;
    const struct device *sensor_temperature;
    const struct device *sensor_acceleration;
} devices_t;

typedef struct _sensors_t {
    data_t data;
    data_t ringbuffer[RINGBUFFER_SIZE];

    devices_t devices;
    
    unsigned int period;
    unsigned int ringbuffer_index;
} sensors_t;

typedef struct _board_t {
    rgbled_t led;
    sensors_t sensors;
    const struct device *console;
    char command_buffer[COMMAND_BUFFER_SIZE];
    
    _board_status_t status;
    _board_mode_t mode;
    _sensor_type_t sensor_type;
} board_t;

extern board_t board;

#endif