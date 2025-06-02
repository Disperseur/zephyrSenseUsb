#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/printk.h>
#include "global.h"
#include "node_led.h"
#include "node_cmd.h"
#include "node_measures.h"

board_t board = {
    .led = {
        .led_red   					= GPIO_DT_SPEC_GET(LED_RED, gpios),
        .led_green 					= GPIO_DT_SPEC_GET(LED_GREEN, gpios),
        .led_blue  					= GPIO_DT_SPEC_GET(LED_BLUE, gpios)
    },
    .sensors = {
        .devices = {
            .sensor_pressure     	= DEVICE_DT_GET_ONE(st_lps22hb_press),
            .sensor_temperature  	= DEVICE_DT_GET_ONE(renesas_hs300x),
            .sensor_acceleration 	= DEVICE_DT_GET_ONE(bosch_bmi270)
        },
        .period                     = 500,
        .ringbuffer_index           = 0
    },
    .console 						= DEVICE_DT_GET(DT_CHOSEN(zephyr_console)),
	
    .status  						= STOPPED,
	.mode							= RINGBUFFER,
	.sensor_type					= ALL
};

int main(void)
{
    node_led_init();

    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        printk("Failed to start USB\n");
        while(1);
    }

    while (!dtr) {
        uart_line_ctrl_get(board.console, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    node_cmd_init();

    printk("STARTING\n");
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
    printk("STARTED\n");

    setup_sensor_acceleration(board.sensors.devices.sensor_acceleration);

    node_measures_init();

    while (1) {
        k_sleep(K_FOREVER);
    }
}