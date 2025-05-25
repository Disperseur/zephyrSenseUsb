#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>


#include <math.h>

#define LED_RED 	DT_ALIAS(led0)
#define LED_GREEN 	DT_ALIAS(led1)
#define LED_BLUE 	DT_ALIAS(led2)



LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);


typedef enum {RUNNING, WAITING, FAULT} _board_status_t;
_board_status_t board_status = RUNNING;


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

data_t sensors_data;
struct sensor_value val_pressure;
struct sensor_value val_temperature;
struct sensor_value val_humidity;
struct sensor_value acc[3], gyr[3];

const struct gpio_dt_spec led_red 			= GPIO_DT_SPEC_GET(LED_RED, gpios);
const struct gpio_dt_spec led_green 		= GPIO_DT_SPEC_GET(LED_GREEN, gpios);
const struct gpio_dt_spec led_blue 			= GPIO_DT_SPEC_GET(LED_BLUE, gpios);

const struct device *const dev 				= DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
const struct device *sensor_pressure 		= DEVICE_DT_GET_ONE(st_lps22hb_press);
const struct device *sensor_temperature 	= DEVICE_DT_GET_ONE(renesas_hs300x);
const struct device *sensor_acceleration 	= DEVICE_DT_GET_ONE(bosch_bmi270);

// timers
struct k_timer timer_led;



// // threads
// K_THREAD_STACK_DEFINE(board_status_led_thread_stack_area, 1024);
// struct k_thread board_status_led_thread_data;



void setup_sensor_acceleration(const struct device *sensor_acceleration);
void _cb_board_status_led(struct k_timer *tim);

int main(void)
{
	int ret_sensor_pressure, ret_sensor_temperature, ret_sensor_acceleration;
	// config capteurs et console
	
	uint32_t dtr = 0;
	if (usb_enable(NULL)) {
		LOG_ERR("Failed to start USB");
		while(1);
	}

	
	while (!(gpio_is_ready_dt(&led_red) && gpio_is_ready_dt(&led_green) && gpio_is_ready_dt(&led_blue))) {
		LOG_ERR("RGB LED not ready");
		k_sleep(K_MSEC(100));
	}

	gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	LOG_INF("Waiting for sensors to be ready...");

	while (!device_is_ready(sensor_pressure)) {
		LOG_ERR("Device %s is not ready\n", sensor_pressure->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(sensor_temperature)) {
		LOG_ERR("Device %s is not ready\n", sensor_temperature->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(sensor_acceleration)) {
		LOG_ERR("Device %s is not ready\n", sensor_acceleration->name);
		k_sleep(K_SECONDS(1));
	}

	LOG_INF("Sensors ready.");

	setup_sensor_acceleration(sensor_acceleration);


	// timers setup and startup
	k_timer_init(&timer_led, _cb_board_status_led, NULL);
	k_timer_start(&timer_led, K_MSEC(300), K_MSEC(300));

	// old thread	
	// k_tid_t my_tid = k_thread_create(&board_status_led_thread_data, board_status_led_thread_stack_area, K_THREAD_STACK_SIZEOF(board_status_led_thread_stack_area), _board_status_led, NULL, NULL, NULL, 10, 0, K_NO_WAIT);


	while (1) {
		//mesures
    	ret_sensor_pressure= sensor_sample_fetch(sensor_pressure);
		ret_sensor_temperature = sensor_sample_fetch(sensor_temperature);
		ret_sensor_acceleration = sensor_sample_fetch(sensor_acceleration);

		if(ret_sensor_pressure != 0) LOG_ERR("failed to fetch pressure sensor: %d", ret_sensor_pressure);
		else {
			ret_sensor_pressure = sensor_channel_get(sensor_pressure, SENSOR_CHAN_PRESS, &val_pressure);
			
			if(ret_sensor_pressure != 0) LOG_ERR("failed to get pressure: %d", ret_sensor_pressure);
		}

		
		if(ret_sensor_temperature != 0) LOG_ERR("failed to fetch temperature sensor: %d", ret_sensor_temperature);
		else {
			ret_sensor_temperature = sensor_channel_get(sensor_temperature, SENSOR_CHAN_AMBIENT_TEMP, &val_temperature);

			if(ret_sensor_temperature != 0) LOG_ERR("failed to get temperature: %d", ret_sensor_temperature);
		
			ret_sensor_temperature = sensor_channel_get(sensor_temperature, SENSOR_CHAN_HUMIDITY, &val_humidity);
			
			if(ret_sensor_temperature != 0) LOG_ERR("failed to get humidity: %d", ret_sensor_temperature);
		}

		
		if(ret_sensor_acceleration != 0) LOG_ERR("failed to fetch acceleration sensor: %d", ret_sensor_acceleration);
		else {
			ret_sensor_acceleration = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, acc);
			
			if(ret_sensor_acceleration != 0) LOG_ERR("failed to get acc: %d", ret_sensor_acceleration);
			
			ret_sensor_acceleration = sensor_channel_get(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, gyr);
			
			if(ret_sensor_acceleration != 0) LOG_ERR("failed to get gyr: %d", ret_sensor_acceleration);
		}

		// conversion
		sensors_data.temperature 	= sensor_value_to_milli(&val_temperature);
		sensors_data.humidity 		= sensor_value_to_milli(&val_humidity);
		sensors_data.pressure 		= sensor_value_to_milli(&val_pressure);
		sensors_data.altitude 		= 1000.0 * 44330.0 * ( 1.0 - pow(sensor_value_to_double(&val_pressure)/101.325, 1/5.255) );
		sensors_data.accel.ax 		= sensor_value_to_milli(&acc[0]);
		sensors_data.accel.ay 		= sensor_value_to_milli(&acc[1]);
		sensors_data.accel.az 		= sensor_value_to_milli(&acc[2]);
		sensors_data.gyro.gx 		= sensor_value_to_milli(&gyr[0]);
		sensors_data.gyro.gy 		= sensor_value_to_milli(&gyr[1]);
		sensors_data.gyro.gz 		= sensor_value_to_milli(&gyr[2]);


		// affichage en millieme de l'unite correspondante pour eviter les flottants
		printk("TEMPERATURE %lld\n", sensors_data.temperature);
		printk("HUMIDITY %lld\n", sensors_data.humidity);
		printk("PRESSURE %lld\n", sensors_data.pressure);
		printk("ALTITUDE %lld\n", sensors_data.altitude);
		printk("ACCEL_LIN %lld %lld %lld\n", sensors_data.accel.ax, sensors_data.accel.ay, sensors_data.accel.az);
		printk("ACCEL_ROT %lld %lld %lld\n", sensors_data.gyro.gx,  sensors_data.gyro.gy,  sensors_data.gyro.gz);		
		printk("\n");

		k_sleep(K_SECONDS(1));

		board_status = FAULT;
	}
}





void _cb_board_status_led(struct k_timer *tim) {
	//callback to display the status of the program on the RGB led

	switch (board_status)
	{
	case RUNNING:
		gpio_pin_set_dt(&led_red, 0);
		gpio_pin_set_dt(&led_blue, 0);

		gpio_pin_toggle_dt(&led_green);
		break;

	case WAITING:
		gpio_pin_set_dt(&led_red, 0);
		gpio_pin_set_dt(&led_blue, 0);

		gpio_pin_set_dt(&led_green, 1);
		break;

	case FAULT:
		gpio_pin_set_dt(&led_green, 0);
		gpio_pin_set_dt(&led_blue, 0);

		gpio_pin_toggle_dt(&led_red);
		break;
	
	default:
		gpio_pin_set_dt(&led_red, 0);
		gpio_pin_set_dt(&led_green, 0);
		gpio_pin_set_dt(&led_blue, 1);
		break;
	}
}


void setup_sensor_acceleration(const struct device *sensor_acceleration) {
	struct sensor_value full_scale, sampling_freq, oversampling;

	// config acc lin sensor
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

	// config acc rot sensor
	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(sensor_acceleration, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
}