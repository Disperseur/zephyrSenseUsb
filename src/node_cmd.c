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
                    _handler_cmd, NULL, NULL, NULL, THREAD_CMD_PRIO, 0, K_NO_WAIT);

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
        else if(strncmp(board.command_buffer, "GET PERIOD", 10) == 0) {
            printk("PERIOD %d\n", board.sensors.period);
        }
        else if(strncmp(board.command_buffer, "GET RINGBUFFER", 14) == 0) {
            // afficher ringbuffer
            for(int i = 0; i<RINGBUFFER_SIZE; i++) {
                printk("%d %d TEMPERATURE %lld\n",         i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].temperature);
                printk("%d %d HUMIDITY %lld\n",            i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].humidity);
                printk("%d %d PRESSURE %lld\n",            i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].pressure);
                printk("%d %d ALTITUDE %lld\n",            i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].altitude);
                printk("%d %d ACCEL_LIN %lld %lld %lld\n", i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].accel.ax, board.sensors.ringbuffer[i].accel.ay, board.sensors.ringbuffer[i].accel.az);
                printk("%d %d ACCEL_ROT %lld %lld %lld\n", i, board.sensors.ringbuffer[i].timestamp, board.sensors.ringbuffer[i].gyro.gx, board.sensors.ringbuffer[i].gyro.gy, board.sensors.ringbuffer[i].gyro.gz);
                // printk("\n");
            }
        }
        else if(strncmp(board.command_buffer, "GET TRIGGER1", 12) == 0) {
            printk("TRIGGER1 EN %d\n", board.trigger1.trigger_en);
            printk("TRIGGER1 TRIGGERED_SENSOR ");
            switch (board.trigger1.triggered_sensor)
            {
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

            case NONE:
                printk("NONE\n");
                break;
            
            default:
                printk("UNKNOWN\n");
                break;
            }
            printk("TRIGGER1 COMP ");
            switch (board.trigger1.comp)
            {
            case SUP:
                printk("SUP\n");
                break;
            
            case INF:
                printk("INF\n");
                break;
            
            default:
                printk("UNKNOWN\n");
                break;
            }
            printk("TRIGGER1 FLOOR %d\n", board.trigger1.floor);
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
                printk("DONE");
            }
        }
        else if(strncmp(board.command_buffer, "STOP", 4) == 0) {
            if(board.status == RUNNING) {
                k_timer_stop(&timer_measures);
                board.status = STOPPED;
                printk("DONE");
            }
        }
        else if(strncmp(board.command_buffer, "SET", 3) == 0 && board.status == STOPPED) {
            if(strstr(board.command_buffer, "MODE") != NULL) {
                if(strstr(board.command_buffer, "STREAMING") != NULL) {
                    board.mode = STREAMING;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ONESHOT") != NULL) {
                    board.mode = ONESHOT;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "RINGBUFFER") != NULL) {
                    board.mode = RINGBUFFER;
                    printk("DONE");
                }
            }

            if(strstr(board.command_buffer, "SENSORS") != NULL) {
                if(strstr(board.command_buffer, "NONE") != NULL) {
                    board.sensor_type = NONE;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "TEMPERATURE") != NULL) {
                    board.sensor_type = TEMPERATURE;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "PRESSURE") != NULL) {
                    board.sensor_type = PRESSURE;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "HUMIDITY") != NULL) {
                    board.sensor_type = HUMIDITY;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ALTITUDE") != NULL) {
                    board.sensor_type = ALTITUDE;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ACCELERATION") != NULL) {
                    board.sensor_type = ACCELERATION;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "GYROSCOPE") != NULL) {
                    board.sensor_type = GYROSCOPE;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "MAGFIELD") != NULL) {
                    board.sensor_type = MAGFIELD;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ALL") != NULL) {
                    board.sensor_type = ALL;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ENV") != NULL) {
                    board.sensor_type = ENV;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "MOTION") != NULL) {
                    board.sensor_type = MOTION;
                    printk("DONE");
                }
            }

            if(strstr(board.command_buffer, "PERIOD") != NULL) {
                int period = atoi(board.command_buffer+11);
                if(0 <= period && period <= 10000) {
                    board.sensors.period = period;
                    printk("DONE\n");
                }
            }

            if(strstr(board.command_buffer, "TRIGGER1") != NULL) {
                if(strstr(board.command_buffer, "NONE") != NULL) {
                    board.trigger1.triggered_sensor = NONE;
                    board.trigger1.trigger_en = false;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "TEMPERATURE") != NULL) {
                    board.trigger1.triggered_sensor = TEMPERATURE;
                    board.trigger1.trigger_en = true;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "PRESSURE") != NULL) {
                    board.trigger1.triggered_sensor = PRESSURE;
                    board.trigger1.trigger_en = true;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "HUMIDITY") != NULL) {
                    board.trigger1.triggered_sensor = HUMIDITY;
                    board.trigger1.trigger_en = true;
                    printk("DONE");
                }
                if(strstr(board.command_buffer, "ALTITUDE") != NULL) {
                    board.trigger1.triggered_sensor = ALTITUDE;
                    board.trigger1.trigger_en = true;
                    printk("DONE");
                }
                
                // traitement de la limite dans une commande separee
                if(strstr(board.command_buffer, "FLOOR") != NULL) {
                    int floor = atoi(board.command_buffer+19);
                    if(floor != 0) {
                        board.trigger1.floor = floor;
                        printk("DONE\n");
                    }
                }
            }
        }

        // command buffer erase
        for(int i = 0; i<COMMAND_BUFFER_SIZE; i++) {
            board.command_buffer[i] = 0;
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