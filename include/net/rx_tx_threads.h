#pragma once
#include "packet_queue.h"

void start_rx_tx_threads(void);
packet_queue_t* get_rx_queue(void);
packet_queue_t* get_tx_queue(void);