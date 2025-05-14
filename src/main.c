#include <zephyr/kernel.h>
// #include <zephyr/device.h>

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>


#define DEBUG


int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;
	if (usb_enable(NULL)) {
		return 0;
	}

	const struct device *sensor = DEVICE_DT_GET_ONE(st_lps22hb_press);
    if (sensor == NULL) {
        return -1;
    }

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

#ifdef DEBUG
	printk("Debug session started\n");
#endif


	struct sensor_value val;
	
    
	while (1) {
    	int ret = sensor_sample_fetch(sensor);
		ret = sensor_channel_get(sensor, SENSOR_CHAN_PRESS, &val);


		printk("val: %d\n", val.val1);
		k_sleep(K_SECONDS(1));
	}
}
