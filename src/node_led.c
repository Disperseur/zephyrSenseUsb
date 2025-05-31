#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "global.h"
#include "node_led.h"

struct k_timer timer_led;

void node_led_init(void) {
    while (!(gpio_is_ready_dt(&board.led.led_red) &&
             gpio_is_ready_dt(&board.led.led_green) &&
             gpio_is_ready_dt(&board.led.led_blue))) {
        k_sleep(K_MSEC(100));
    }
    gpio_pin_configure_dt(&board.led.led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&board.led.led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&board.led.led_blue, GPIO_OUTPUT_INACTIVE);

    k_timer_init(&timer_led, _cb_timer_status_led, NULL);
    k_timer_start(&timer_led, K_MSEC(300), K_MSEC(300));
}

void _cb_timer_status_led(struct k_timer *tim) {
    switch (board.status) {
    case RUNNING:
        gpio_pin_set_dt(&board.led.led_red, 0);
        gpio_pin_set_dt(&board.led.led_blue, 0);
        gpio_pin_toggle_dt(&board.led.led_green);
        break;
    case STOPPED:
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