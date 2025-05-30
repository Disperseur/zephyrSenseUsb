#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <math.h>
#include "global.h"
#include "node_measures.h"

K_THREAD_STACK_DEFINE(measures_thread_stack_area, 2048);
struct k_thread measures_thread_data;
struct k_sem sem_measures;
struct k_timer timer_measures;

void node_measures_init(void) {
    k_sem_init(&sem_measures, 0, 1);
    k_thread_create(&measures_thread_data, measures_thread_stack_area, K_THREAD_STACK_SIZEOF(measures_thread_stack_area),
                    _handler_measures, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

    k_timer_init(&timer_measures, _cb_timer_measures, NULL);
    k_timer_start(&timer_measures, K_MSEC(board.sensors.period), K_MSEC(board.sensors.period));
}

void _handler_measures(void*, void*, void*) {
    int ret;
    uint32_t timestamp;
    struct sensor_value val_pressure, val_temperature, val_humidity;
    struct sensor_value acc[3], gyr[3];

    while (1) {
        k_sem_take(&sem_measures, K_FOREVER);

        ret = sensor_sample_fetch(board.sensors.devices.sensor_pressure);
        if(ret != 0) printk("failed to fetch pressure sensor: %d\n", ret);
        else {
            ret = sensor_channel_get(board.sensors.devices.sensor_pressure, SENSOR_CHAN_PRESS, &val_pressure);
            if(ret != 0) printk("failed to get pressure: %d\n", ret);
        }

        ret = sensor_sample_fetch(board.sensors.devices.sensor_temperature);
        if(ret != 0) printk("failed to fetch temperature sensor: %d\n", ret);
        else {
            ret = sensor_channel_get(board.sensors.devices.sensor_temperature, SENSOR_CHAN_AMBIENT_TEMP, &val_temperature);
            if(ret != 0) printk("failed to get temperature: %d\n", ret);
            ret = sensor_channel_get(board.sensors.devices.sensor_temperature, SENSOR_CHAN_HUMIDITY, &val_humidity);
            if(ret != 0) printk("failed to get humidity: %d\n", ret);
        }

        ret = sensor_sample_fetch(board.sensors.devices.sensor_acceleration);
        if(ret != 0) printk("failed to fetch acceleration sensor: %d\n", ret);
        else {
            ret = sensor_channel_get(board.sensors.devices.sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, acc);
            if(ret != 0) printk("failed to get acc: %d\n", ret);
            ret = sensor_channel_get(board.sensors.devices.sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, gyr);
            if(ret != 0) printk("failed to get gyr: %d\n", ret);
        }

        timestamp = k_uptime_get_32();

        board.sensors.data.temperature  = sensor_value_to_milli(&val_temperature);
        board.sensors.data.humidity     = sensor_value_to_milli(&val_humidity);
        board.sensors.data.pressure     = sensor_value_to_milli(&val_pressure);
        board.sensors.data.altitude     = 1000.0 * 44330.0 * ( 1.0 - pow(sensor_value_to_double(&val_pressure)/101.325, 1/5.255) );
        board.sensors.data.accel.ax     = sensor_value_to_milli(&acc[0]);
        board.sensors.data.accel.ay     = sensor_value_to_milli(&acc[1]);
        board.sensors.data.accel.az     = sensor_value_to_milli(&acc[2]);
        board.sensors.data.gyro.gx      = sensor_value_to_milli(&gyr[0]);
        board.sensors.data.gyro.gy      = sensor_value_to_milli(&gyr[1]);
        board.sensors.data.gyro.gz      = sensor_value_to_milli(&gyr[2]);

        printk("%d TEMPERATURE %lld\n", timestamp, board.sensors.data.temperature);
        printk("%d HUMIDITY %lld\n", timestamp, board.sensors.data.humidity);
        printk("%d PRESSURE %lld\n", timestamp, board.sensors.data.pressure);
        printk("%d ALTITUDE %lld\n", timestamp, board.sensors.data.altitude);
        printk("%d ACCEL_LIN %lld %lld %lld\n", timestamp, board.sensors.data.accel.ax, board.sensors.data.accel.ay, board.sensors.data.accel.az);
        printk("%d ACCEL_ROT %lld %lld %lld\n", timestamp, board.sensors.data.gyro.gx,  board.sensors.data.gyro.gy,  board.sensors.data.gyro.gz);        
        printk("\n");
    }
}

void _cb_timer_measures(struct k_timer *tim) {
    k_sem_give(&sem_measures);
}

void setup_sensor_acceleration(const struct device *sensor_acceleration) {
    struct sensor_value full_scale, sampling_freq, oversampling;

    full_scale.val1 = 2; full_scale.val2 = 0;
    sampling_freq.val1 = 100; sampling_freq.val2 = 0;
    oversampling.val1 = 1; oversampling.val2 = 0;

    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

    full_scale.val1 = 500; full_scale.val2 = 0;
    sampling_freq.val1 = 100; sampling_freq.val2 = 0;
    oversampling.val1 = 1; oversampling.val2 = 0;

    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
    sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
}