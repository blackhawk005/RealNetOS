#ifndef IPC_H
#define IPC_H

#define MAILBOX_SIZE 8

typedef struct {
    int from;
    char data[32];
} message_t;

typedef struct {
    message_t messages[MAILBOX_SIZE];
    int head, tail, count;
} mailbox_t;

int send(int to_id, const char* data);
int receive(message_t* out);

#endif