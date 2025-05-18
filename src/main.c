#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>


#include <math.h>


#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);


typedef struct _accel_t {
	int64_t ax;
	int64_t ay;
	int64_t az;
} accel_t;

typedef struct _gyro_t {
	int64_t gx;
	int64_t gy;
	int64_t gz;
} gyro_t;

typedef struct _data_t {
	int64_t temperature;
	int64_t humidity;
	int64_t pressure;
	int64_t altitude;
	accel_t accel;
	gyro_t  gyro;
} data_t;


int main(void)
{
	// config capteurs et console
	static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

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

	
	while (!gpio_is_ready_dt(&led)) {
		printk("led is not ready\n");
		k_sleep(K_SECONDS(1));
	}

	while (!dtr) {
		/* Poll if the DTR flag was set */
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_SECONDS(1));
	}

	LOG_INF("Waiting for sensors to be ready...");

	while (!device_is_ready(sensor_pressure)) {
		printk("Device %s is not ready\n", sensor_pressure->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(sensor_temperature)) {
		printk("Device %s is not ready\n", sensor_temperature->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(sensor_acceleration)) {
		printk("Device %s is not ready\n", sensor_acceleration->name);
		k_sleep(K_SECONDS(1));
	}
	// while (!device_is_ready(sensor_magfield)) {
	// 	printf("Device %s is not ready\n", sensor_magfield->name);
	// 	k_sleep(K_SECONDS(1));
	// }

	LOG_INF("Sensors ready.");

	


	data_t sensors_data;
	struct sensor_value val_pressure;
	struct sensor_value val_temperature;
	struct sensor_value val_humidity;
	struct sensor_value acc[3], gyr[3];
	// struct sensor_value mag[3];

	struct sensor_value full_scale, sampling_freq, oversampling;
	
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

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
    
	while (1) {
		ret = gpio_pin_toggle_dt(&led);

		//mesures
    	ret = sensor_sample_fetch(sensor_pressure);
		if(ret != 0) LOG_ERR("failed to fetch pressure sensor: %d", ret);
		else {
			ret = sensor_channel_get(sensor_pressure, SENSOR_CHAN_PRESS, &val_pressure);
			
			if(ret != 0) LOG_ERR("failed to get pressure: %d", ret);
		}

		ret = sensor_sample_fetch(sensor_temperature);
		if(ret != 0) LOG_ERR("failed to fetch temperature sensor: %d", ret);
		else {
			ret = sensor_channel_get(sensor_temperature, SENSOR_CHAN_AMBIENT_TEMP, &val_temperature);

			if(ret != 0) LOG_ERR("failed to get temperature: %d", ret);
		
			ret = sensor_channel_get(sensor_temperature, SENSOR_CHAN_HUMIDITY, &val_humidity);
			
			if(ret != 0) LOG_ERR("failed to get humidity: %d", ret);
		}

		ret = sensor_sample_fetch(sensor_acceleration);
		if(ret != 0) LOG_ERR("failed to fetch acceleration sensor: %d", ret);
		else {
			ret = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, acc);
			
			if(ret != 0) LOG_ERR("failed to get acc: %d", ret);
			
			ret = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, gyr);
			
			if(ret != 0) LOG_ERR("failed to get gyr: %d", ret);
		}

		// ret = sensor_sample_fetch(sensor_magfield);
		// ret = sensor_channel_get(sensor_magfield, SENSOR_CHAN_MAGN_XYZ, mag);


		// conversion
		sensors_data.temperature = sensor_value_to_milli(&val_temperature);
		sensors_data.humidity = sensor_value_to_milli(&val_humidity);
		sensors_data.pressure = sensor_value_to_milli(&val_pressure);
		sensors_data.altitude = 1000.0 * 44330.0 * ( 1.0 - pow(sensor_value_to_double(&val_pressure)/101.325, 1/5.255) );
		sensors_data.accel.ax = sensor_value_to_milli(&acc[0]);
		sensors_data.accel.ay = sensor_value_to_milli(&acc[1]);
		sensors_data.accel.az = sensor_value_to_milli(&acc[2]);
		sensors_data.gyro.gx = sensor_value_to_milli(&gyr[0]);
		sensors_data.gyro.gy = sensor_value_to_milli(&gyr[1]);
		sensors_data.gyro.gz = sensor_value_to_milli(&gyr[2]);


		// affichage en millieme de l'unite correspondante pour eviter les flottants
		printk("TEMPERATURE %lld\n", sensors_data.temperature);
		printk("HUMIDITY %lld\n", sensors_data.humidity);
		printk("PRESSURE %lld\n", sensors_data.pressure);
		printk("ALTITUDE %lld\n", sensors_data.altitude);
		printk("ACCEL_LIN %lld %lld %lld\n", sensors_data.accel.ax, sensors_data.accel.ay, sensors_data.accel.az);
		printk("ACCEL_ROT %lld %lld %lld\n", sensors_data.gyro.gx,  sensors_data.gyro.gy,  sensors_data.gyro.gz);
		// printk("MX: %d.%06d; MY: %d.%06d; MZ: %d.%06d;\n", mag[0].val1, mag[0].val2, mag[1].val1, mag[1].val2, mag[2].val1, mag[2].val2);
		
		printk("\n");

		k_sleep(K_SECONDS(1));
	}
}
