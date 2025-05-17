#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;
	if (usb_enable(NULL)) {
		return 0;
	}

	const struct device *sensor_pressure = DEVICE_DT_GET_ONE(st_lps22hb_press);
    if (sensor_pressure == NULL) {
        return -1;
    }

	const struct device *sensor_temperature = DEVICE_DT_GET_ONE(renesas_hs300x);
    if (sensor_temperature == NULL) {
        return -1;
    }

	const struct device *sensor_acceleration = DEVICE_DT_GET_ONE(bosch_bmi270);
    if (sensor_acceleration == NULL) {
        return -1;
    }

	// const struct device *sensor_magfield = DEVICE_DT_GET_ONE(bosch_bmm150);
    // if (sensor_magfield == NULL) {
    //     return -1;
    // }

	

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	



	struct sensor_value val_pressure;
	struct sensor_value val_temperature;
	struct sensor_value val_humidity;
	struct sensor_value acc[3], gyr[3], mag[3];

	struct sensor_value full_scale, sampling_freq, oversampling;

	// while (!device_is_ready(sensor_magfield)) {
	// 	printf("Device %s is not ready\n", sensor_magfield->name);
	// 	k_sleep(K_SECONDS(1));
	// }
	
	while (!device_is_ready(sensor_acceleration)) {
		printk("Device %s is not ready\n", sensor_acceleration->name);
		k_sleep(K_SECONDS(1));
	}

	

	// config acc sensor
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

		
	int ret;
    
	while (1) {
    	ret = sensor_sample_fetch(sensor_pressure);
		if(ret != 0) {
			LOG_ERR("failed to fetch pressure sensor: %d", ret);
		}
		ret = sensor_channel_get(sensor_pressure, SENSOR_CHAN_PRESS, &val_pressure);
		if(ret != 0) {
			LOG_ERR("failed to get pressure: %d", ret);
		}

		ret = sensor_sample_fetch(sensor_temperature);
		if(ret != 0) {
			LOG_ERR("failed to fetch temperature sensor: %d", ret);
		}
		ret = sensor_channel_get(sensor_temperature, SENSOR_CHAN_AMBIENT_TEMP, &val_temperature);
		if(ret != 0) {
			LOG_ERR("failed to get temperature: %d", ret);
		}
		ret = sensor_channel_get(sensor_temperature, SENSOR_CHAN_HUMIDITY, &val_humidity);
		if(ret != 0) {
			LOG_ERR("failed to get humidity: %d", ret);
		}


		ret = sensor_sample_fetch(sensor_acceleration);
		if(ret != 0) {
			LOG_ERR("failed to fetch acceleration sensor: %d", ret);
		}
		ret = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, acc);
		if(ret != 0) {
			LOG_ERR("failed to get acc: %d", ret);
		}
		ret = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, gyr);
		if(ret != 0) {
			LOG_ERR("failed to get gyr: %d", ret);
		}

		// ret = sensor_sample_fetch(sensor_magfield);
		// ret = sensor_channel_get(sensor_magfield, SENSOR_CHAN_MAGN_XYZ, mag);


		printk("AX: %d.%06d; AY: %d.%06d; AZ: %d.%06d;\n", acc[0].val1, acc[0].val2, acc[1].val1, acc[1].val2, acc[2].val1, acc[2].val2);
		printk("GX: %d.%06d; GY: %d.%06d; GZ: %d.%06d;\n", gyr[0].val1, gyr[0].val2, gyr[1].val1, gyr[1].val2, gyr[2].val1, gyr[2].val2);
		// printk("MX: %d.%06d; MY: %d.%06d; MZ: %d.%06d;\n", mag[0].val1, mag[0].val2, mag[1].val1, mag[1].val2, mag[2].val1, mag[2].val2);


		printk("pressure: %d kPa\n", val_pressure.val1);
		printk("temperature: %d *C\n", val_temperature.val1);
		printk("humidity: %d \n", val_humidity.val1);

		k_sleep(K_SECONDS(1));
	}
}
