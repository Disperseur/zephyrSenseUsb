#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "global.h"
#include "node_cmd.h"
#include "node_measures.h"
#include "string.h"

K_THREAD_STACK_DEFINE(cmd_thread_stack_area, 1024);
struct k_thread cmd_thread_data;
struct k_sem sem_cmd;

void node_cmd_init(void) {
    k_sem_init(&sem_cmd, 0, 1);
    k_thread_create(&cmd_thread_data, cmd_thread_stack_area, K_THREAD_STACK_SIZEOF(cmd_thread_stack_area),
                    _handler_cmd, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    uart_irq_callback_set(board.console, _cb_uart_rx);
    uart_irq_rx_enable(board.console);
}

void _handler_cmd(void*, void*, void*) {
    while(1) {
        k_sem_take(&sem_cmd, K_FOREVER);

        /*traitement de la commande*/
        printk("ACK\n");

        if(strncmp(board.command_buffer, "GET STATUS", 10) == 0) {
            printk("STATUS ");

            switch (board.status)
            {
            case RUNNING:
                printk("RUNNING\n");
                break;

            case STOPPED:
                printk("STOPPED\n");
                break;

            case FAULT:
                printk("FAULT\n");
                break;
            
            default:
                printk("UNKNOWN\n");
                break;
            }

        }
        else if(strncmp(board.command_buffer, "GET MODE", 8) == 0) {
            printk("MODE ");

            switch (board.mode)
            {
            case STREAMING:
                printk("STREAMING\n");
                break;

            case ONESHOT:
                printk("ONESHOT\n");
                break;

            case RINGBUFFER:
                printk("RINGBUFFER\n");
                break;
            
            default:
                break;
            }

        }
        else if(strncmp(board.command_buffer, "GET SENSORS", 11) == 0) {
            printk("SENSORS ");

            switch (board.sensor_type)
            {
            case NONE:
                printk("NONE\n");
                break;

            case TEMPERATURE:
                printk("TEMPERATURE\n");
                break;
            
            case PRESSURE:
                printk("PRESSURE\n");
                break;

            case HUMIDITY:
                printk("HUMIDITY\n");
                break;

            case ALTITUDE:
                printk("ALTITUDE\n");
                break;

            case ACCELERATION:
                printk("ACCELERATION\n");
                break;

            case GYROSCOPE:
                printk("NOGYROSCOPENE\n");
                break;

            case MAGFIELD:
                printk("MAGFIELD\n");
                break;

            case ALL:
                printk("ALL\n");
                break;

            case ENV:
                printk("ENV\n");
                break;

            case MOTION:
                printk("MOTION\n");
                break;
            
            default:
                printk("UNKNOWN\n");
                break;
            }
        }
        else if(strncmp(board.command_buffer, "START", 5) == 0) {
            if(board.status == STOPPED) {
                    if(board.mode == STREAMING || board.mode == RINGBUFFER) {
                        k_timer_start(&timer_measures, K_MSEC(board.sensors.period), K_MSEC(board.sensors.period)); // demarre le timer qui give le semaphore periodiquement
                        board.status = RUNNING;
                    }
                    else if(board.mode == ONESHOT) {
                        k_sem_give(&sem_measures); // give directement le semaphore pour faire une mesure
                    }
                
            }
        }
        else if(strncmp(board.command_buffer, "STOP", 4) == 0) {
            if(board.status == RUNNING) {
                k_timer_stop(&timer_measures);
                board.status = STOPPED;
            }
        }
        else if(strncmp(board.command_buffer, "SET", 3) == 0) {
            if(strstr(board.command_buffer, "MODE") != NULL) {
                if(strstr(board.command_buffer, "STREAMING") != NULL) {
                    board.mode = STREAMING;
                }
                if(strstr(board.command_buffer, "ONESHOT") != NULL) {
                    board.mode = ONESHOT;
                }
                if(strstr(board.command_buffer, "RINGBUFFER") != NULL) {
                    board.mode = RINGBUFFER;
                }
            }
        }
    }
}

void _cb_uart_rx(const struct device *console, void *user_data) {
    if(uart_irq_rx_ready(console)) {
        int i = 0;
        while(uart_fifo_read(console, &board.command_buffer[i], 1)) {
            i++;
        }
    }
    k_sem_give(&sem_cmd);
}