#include "../include/user/sensor_task.h"
#include "../include/syscall.h"
#include "../include/uart.h"
#include "../include/lib.h"

// Simulates incrementing sensor values from 0 to 99
static int fake_sensor_reading() {
    static int value = 0;
    return (value = (value + 1) % 100);
}

void start_sensor_polling_task() {
    while (1) {
        int sensor_value = fake_sensor_reading();
        char buf[64] = "Sensor Value: ";

        // Append sensor_value to buf
        int_to_str(sensor_value, buf + 14);
        int len = 0;
        while (buf[len] != '\0') len++;
        buf[len++] = '\n';
        buf[len] = '\0';

        sys_write(1, buf);       // Print to UART
        sys_sleep(10);           // Sleep for 10 ticks
    }
}
