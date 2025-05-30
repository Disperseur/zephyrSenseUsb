/*
Pour avoir le temps : k_uptime_get_32()
*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <math.h>



#define LED_RED 	DT_ALIAS(led0)
#define LED_GREEN 	DT_ALIAS(led1)
#define LED_BLUE 	DT_ALIAS(led2)

#define COMMAND_BUFFER_SIZE 100



typedef enum {RUNNING, WAITING, FAULT} _board_status_t;


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

typedef struct _rgbled_t {
	struct gpio_dt_spec led_red;
	struct gpio_dt_spec led_green;
	struct gpio_dt_spec led_blue;
} rgbled_t;

typedef struct _devices_t {
	const struct device *sensor_pressure;
	const struct device *sensor_temperature;
	const struct device *sensor_acceleration;
} devices_t;

typedef struct _sensors_t {
	data_t data;
	devices_t devices;

	unsigned int period;
} sensors_t;

typedef struct _board_t {
	rgbled_t led;
	sensors_t sensors;

	const struct device *console;

	char command_buffer[COMMAND_BUFFER_SIZE];

	_board_status_t status;
} board_t;







board_t board = {
	.led = {
		.led_red 					= GPIO_DT_SPEC_GET(LED_RED, gpios),
		.led_green 					= GPIO_DT_SPEC_GET(LED_GREEN, gpios),
		.led_blue 					= GPIO_DT_SPEC_GET(LED_BLUE, gpios)
	},

	.sensors = {
		.devices = {
			.sensor_pressure 		= DEVICE_DT_GET_ONE(st_lps22hb_press),
			.sensor_temperature 	= DEVICE_DT_GET_ONE(renesas_hs300x),
			.sensor_acceleration 	= DEVICE_DT_GET_ONE(bosch_bmi270)
		},

		.period 					= 500
	},

	.console 						= DEVICE_DT_GET(DT_CHOSEN(zephyr_console)),
	.status							= RUNNING
};





// threads
K_THREAD_STACK_DEFINE(cmd_thread_stack_area, 1024);
K_THREAD_STACK_DEFINE(measures_thread_stack_area, 2048);

struct k_timer timer_led;
struct k_timer timer_measures;

struct k_thread cmd_thread_data;
struct k_thread measures_thread_data;

struct k_sem sem_cmd;
struct k_sem sem_measures;





// typedef struct _node_t {
// 	struct k_sem node_sem;
// 	struct k_timer node_tim;
// 	struct k_thread node_thread_data;
// 	k_timer_expiry_t node_tim_cb;
// 	k_thread_entry_t node_handler;

// } node_t;

// node_t cmd_node = {
// 	.node_tim_cb = _cb_uart_rx,
// 	.node_handler = _handler_cmd
// }










void setup_sensor_acceleration(const struct device *sensor_acceleration);

void _handler_cmd(void*, void*, void*);
void _handler_measures(void*, void*, void*);

void _cb_timer_status_led(struct k_timer *tim);
void _cb_timer_measures(struct k_timer *tim);
void _cb_uart_rx(const struct device *console, void *user_data);




int main(void)
{
	// on config en premier la led pour avoir un retour dans le max de cas
	while (!(gpio_is_ready_dt(&board.led.led_red) && gpio_is_ready_dt(&board.led.led_green) && gpio_is_ready_dt(&board.led.led_blue))) {
		k_sleep(K_MSEC(100));
	}

	gpio_pin_configure_dt(&board.led.led_red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&board.led.led_green, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&board.led.led_blue, GPIO_OUTPUT_INACTIVE);

	// timers setup and startup
	k_timer_init(&timer_led, _cb_timer_status_led, NULL);
	k_timer_start(&timer_led, K_MSEC(300), K_MSEC(300));



	uint32_t dtr = 0;
	if (usb_enable(NULL)) {
		printk("Failed to start USB\n");
		board.status = FAULT;
		while(1);
	}
	board.status = RUNNING;

	/* Poll if the DTR flag was set: there is a client at the other side of the serial port */
	while (!dtr) {
		/* Poll if the DTR flag was set */
		uart_line_ctrl_get(board.console, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		board.status = FAULT;
		k_sleep(K_MSEC(100));
	}
	board.status = RUNNING;

	// uart irq setup
	uart_irq_callback_set(board.console, _cb_uart_rx);
	uart_irq_rx_enable(board.console);

	printk("Waiting for sensors to be ready...\n");

	while (!device_is_ready(board.sensors.devices.sensor_pressure)) {
		board.status = FAULT;
		printk("Device %s is not ready\n", board.sensors.devices.sensor_pressure->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(board.sensors.devices.sensor_temperature)) {
		printk("Device %s is not ready\n", board.sensors.devices.sensor_temperature->name);
		k_sleep(K_SECONDS(1));
	}
	while (!device_is_ready(board.sensors.devices.sensor_acceleration)) {
		printk("Device %s is not ready\n", board.sensors.devices.sensor_acceleration->name);
		k_sleep(K_SECONDS(1));
	}

	printk("Sensors ready.\n");

	setup_sensor_acceleration(board.sensors.devices.sensor_acceleration);


	k_sem_init(&sem_cmd, 0, 1);
	k_sem_init(&sem_measures, 0, 1);

	
	// thread ids non recuperes car on ne s'attends pas a ce qu'ils se terminent
	k_thread_create(&cmd_thread_data, cmd_thread_stack_area, K_THREAD_STACK_SIZEOF(cmd_thread_stack_area), _handler_cmd, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_create(&measures_thread_data, measures_thread_stack_area, K_THREAD_STACK_SIZEOF(measures_thread_stack_area), _handler_measures, NULL, NULL, NULL, 1, 0, K_NO_WAIT);


	k_timer_init(&timer_measures, _cb_timer_measures, NULL);
	k_timer_start(&timer_measures, K_MSEC(board.sensors.period), K_MSEC(board.sensors.period));
	
}

void _handler_measures(void*, void*, void*) {
	int ret;
	uint32_t timestamp;
	struct sensor_value val_pressure;
	struct sensor_value val_temperature;
	struct sensor_value val_humidity;
	struct sensor_value acc[3], gyr[3];
	

	while (1) {
		k_sem_take(&sem_measures, K_FOREVER);


		ret= sensor_sample_fetch(board.sensors.devices.sensor_pressure);
		
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

		// conversion
		board.sensors.data.temperature 	= sensor_value_to_milli(&val_temperature);
		board.sensors.data.humidity 		= sensor_value_to_milli(&val_humidity);
		board.sensors.data.pressure 		= sensor_value_to_milli(&val_pressure);
		board.sensors.data.altitude 		= 1000.0 * 44330.0 * ( 1.0 - pow(sensor_value_to_double(&val_pressure)/101.325, 1/5.255) );
		board.sensors.data.accel.ax 		= sensor_value_to_milli(&acc[0]);
		board.sensors.data.accel.ay 		= sensor_value_to_milli(&acc[1]);
		board.sensors.data.accel.az 		= sensor_value_to_milli(&acc[2]);
		board.sensors.data.gyro.gx 		= sensor_value_to_milli(&gyr[0]);
		board.sensors.data.gyro.gy 		= sensor_value_to_milli(&gyr[1]);
		board.sensors.data.gyro.gz 		= sensor_value_to_milli(&gyr[2]);


		//affichage en millieme de l'unite correspondante pour eviter les flottants
		printk("%d TEMPERATURE %lld\n", timestamp, board.sensors.data.temperature);
		printk("%d HUMIDITY %lld\n", timestamp, board.sensors.data.humidity);
		printk("%d PRESSURE %lld\n", timestamp, board.sensors.data.pressure);
		printk("%d ALTITUDE %lld\n", timestamp, board.sensors.data.altitude);
		printk("%d ACCEL_LIN %lld %lld %lld\n", timestamp, board.sensors.data.accel.ax, board.sensors.data.accel.ay, board.sensors.data.accel.az);
		printk("%d ACCEL_ROT %lld %lld %lld\n", timestamp, board.sensors.data.gyro.gx,  board.sensors.data.gyro.gy,  board.sensors.data.gyro.gz);		
		printk("\n");
	}
}


void _handler_cmd(void*, void*, void*) {
	//handler des commandes recues par uart dans la variable command_buffer et command_available

	while(1) {
		k_sem_take(&sem_cmd, K_FOREVER); // on attends qu'une commande soit dispo dans le buffer dedie command_buffer

		printk("ACK\n");
		// une commande est dispo, on l'affiche -> il faudra la récup pour switch dessus a posteriori
		printk("%s", board.command_buffer);
	}
}


void _cb_timer_measures(struct k_timer *tim) {
	k_sem_give(&sem_measures);
}

void _cb_timer_status_led(struct k_timer *tim) {
	//callback to display the status of the program on the RGB led

	switch (board.status)
	{
	case RUNNING:
		gpio_pin_set_dt(&board.led.led_red, 0);
		gpio_pin_set_dt(&board.led.led_blue, 0);

		gpio_pin_toggle_dt(&board.led.led_green);
		break;

	case WAITING:
		gpio_pin_set_dt(&board.led.led_red, 0);
		gpio_pin_set_dt(&board.led.led_blue, 0);

		gpio_pin_set_dt(&board.led.led_green, 1);
		break;

	case FAULT:
		gpio_pin_set_dt(&board.led.led_green, 0);
		gpio_pin_set_dt(&board.led.led_blue, 0);

		gpio_pin_toggle_dt(&board.led.led_red);
		break;
	
	default:
		gpio_pin_set_dt(&board.led.led_red, 0);
		gpio_pin_set_dt(&board.led.led_green, 0);
		gpio_pin_set_dt(&board.led.led_blue, 1);
		break;
	}

}

void _cb_uart_rx(const struct device *console, void *user_data) {
	// printk("ACK\n");

	if(uart_irq_rx_ready(console)) {
		// si data a lire dans la fifo uart
		int i = 0;
		while(uart_fifo_read(console, &board.command_buffer[i], 1)) {
			i++;
		}
	}
	k_sem_give(&sem_cmd); // libere le thread de gestion des commandes pour le traitement de la nouvelle commande
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