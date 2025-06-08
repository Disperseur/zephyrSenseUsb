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
    k_sem_init(&sem_measures, 0, 100);
    k_thread_create(&measures_thread_data, measures_thread_stack_area, K_THREAD_STACK_SIZEOF(measures_thread_stack_area),
                    _handler_measures, NULL, NULL, NULL, THREAD_MEASURES_PRIO, 0, K_NO_WAIT);

    k_timer_init(&timer_measures, _cb_timer_measures, NULL);
}

void _handler_measures(void*, void*, void*) {
    int ret;
    struct sensor_value val_pressure, val_temperature, val_humidity;
    struct sensor_value acc[3], gyr[3];
    unsigned int nb_measures_todo = 0;

    while (1) {
        k_sem_take(&sem_measures, K_FOREVER);

        // check si trop rapide
        nb_measures_todo = k_sem_count_get(&sem_measures);
        if(nb_measures_todo > 0) {
            printk("BACKLOG %d\n", nb_measures_todo); // peut etre a afficher systematiquement pour simplifier la gestion cote API
        }

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

        board.sensors.data.timestamp    = k_uptime_get_32();

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

        if(board.mode == ONESHOT || board.mode == STREAMING) {
            int64_t *value;

            if(board.trigger1.trigger_en) {
                // cas trigger allume
                switch (board.trigger1.triggered_sensor)
                {
                case TEMPERATURE:
                    value = &board.sensors.data.temperature;
                    break;

                case PRESSURE:
                    value = &board.sensors.data.pressure;
                    break;

                case HUMIDITY:
                    value = &board.sensors.data.humidity;
                    break;

                case ALTITUDE:
                    value = &board.sensors.data.altitude;
                    break;
                
                default:
                    value = &board.sensors.data.temperature;
                    break;
                }


                if( ((*value > board.trigger1.floor) && (board.trigger1.comp == SUP)) ||
                    ((*value < board.trigger1.floor) && (board.trigger1.comp == INF))
                ) {
                    if(board.sensor_type == TEMPERATURE     || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d TEMPERATURE %lld\n",         board.sensors.data.timestamp, board.sensors.data.temperature);
                    if(board.sensor_type == HUMIDITY        || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d HUMIDITY %lld\n",            board.sensors.data.timestamp, board.sensors.data.humidity);
                    if(board.sensor_type == PRESSURE        || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d PRESSURE %lld\n",            board.sensors.data.timestamp, board.sensors.data.pressure);
                    if(board.sensor_type == ALTITUDE        || board.sensor_type == ALL)                                    printk("%d ALTITUDE %lld\n",            board.sensors.data.timestamp, board.sensors.data.altitude);
                    if(board.sensor_type == ACCELERATION    || board.sensor_type == MOTION  || board.sensor_type == ALL)    printk("%d ACCEL_LIN %lld %lld %lld\n", board.sensors.data.timestamp, board.sensors.data.accel.ax, board.sensors.data.accel.ay, board.sensors.data.accel.az);
                    if(board.sensor_type == GYROSCOPE       || board.sensor_type == MOTION  || board.sensor_type == ALL)    printk("%d ACCEL_ROT %lld %lld %lld\n", board.sensors.data.timestamp, board.sensors.data.gyro.gx,  board.sensors.data.gyro.gy,  board.sensors.data.gyro.gz);        
                    // printk("\n");
                }
            }
            else {
                // pas de trigger
                if(board.sensor_type == TEMPERATURE     || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d TEMPERATURE %lld\n",         board.sensors.data.timestamp, board.sensors.data.temperature);
                if(board.sensor_type == HUMIDITY        || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d HUMIDITY %lld\n",            board.sensors.data.timestamp, board.sensors.data.humidity);
                if(board.sensor_type == PRESSURE        || board.sensor_type == ENV     || board.sensor_type == ALL)    printk("%d PRESSURE %lld\n",            board.sensors.data.timestamp, board.sensors.data.pressure);
                if(board.sensor_type == ALTITUDE        || board.sensor_type == ALL)                                    printk("%d ALTITUDE %lld\n",            board.sensors.data.timestamp, board.sensors.data.altitude);
                if(board.sensor_type == ACCELERATION    || board.sensor_type == MOTION  || board.sensor_type == ALL)    printk("%d ACCEL_LIN %lld %lld %lld\n", board.sensors.data.timestamp, board.sensors.data.accel.ax, board.sensors.data.accel.ay, board.sensors.data.accel.az);
                if(board.sensor_type == GYROSCOPE       || board.sensor_type == MOTION  || board.sensor_type == ALL)    printk("%d ACCEL_ROT %lld %lld %lld\n", board.sensors.data.timestamp, board.sensors.data.gyro.gx,  board.sensors.data.gyro.gy,  board.sensors.data.gyro.gz);        
                // printk("\n");
            }
        }
        else if(board.mode == RINGBUFFER) {
            // mecanisme de trigger pas implemente pour le ringbuffer encore
            board.sensors.ringbuffer[board.sensors.ringbuffer_index%RINGBUFFER_SIZE] = board.sensors.data; // possible source de problemes de pointeurs
            board.sensors.ringbuffer_index++;
        }

        if(board.status == STOPPED && nb_measures_todo == 0) {
            printk("DONE\n");
        }
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