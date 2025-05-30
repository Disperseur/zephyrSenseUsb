#ifndef NODE_CMD_H
#define NODE_CMD_H

#include "global.h"

extern struct k_thread cmd_thread_data;
extern struct k_sem sem_cmd;

void node_cmd_init(void);
void _handler_cmd(void*, void*, void*);
void _cb_uart_rx(const struct device *console, void *user_data);

#endif