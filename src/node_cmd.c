#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "global.h"
#include "node_cmd.h"
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

        // if(strncmp(board.command_buffer, "GET STATUS", 10) == 0) {
        //     printk("STATUS RUNNING\n");
        // }

        printk("%s", board.command_buffer);
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