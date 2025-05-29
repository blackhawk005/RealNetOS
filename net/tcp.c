#include "../include/net/tcp.h"
#include "../include/net/ip.h"
#include "../include/net/net_utils.h"
#include "../include/lib.h"
#include "../include/uart.h"

static tcp_state_t tcp_state = TCP_CLOSED;
static uint32_t local_seq = 0x1000;
static uint32_t remote_seq = 0;
static uint32_t remote_ip = 0;
static uint16_t local_port = 1234;
static uint16_t remote_port = 0;
static int is_listening = 0;

char tcp_rx_buffer[TCP_RX_BUFFER_SIZE];
int tcp_rx_len = 0;

// Internal helper to send a TCP packet

static void tcp_send_packet(const char* data, int len, uint8_t flags){
    char packet[1500];
    tcp_header_t* hdr = (tcp_header_t*)packet;

    hdr->src_port = htons(local_port);
    hdr->dest_port = htons(remote_port);
    hdr->seq_num = htonl(local_seq);
    hdr->ack_num = htonl(remote_seq);
    hdr->data_offset = (TCP_HEADER_SIZE / 4) << 4;
    hdr->flags = flags;
    hdr->window = htons(1024);
    hdr->checksum = 0;
    hdr->urgent_ptr = 0;

    memcpy(packet + TCP_HEADER_SIZE, data, len);

    ip_send(remote_ip, packet, TCP_HEADER_SIZE + len, 6);           // Proto 6 = TCP
    if (flags & (TCP_SYN | TCP_FIN)){
        local_seq += 1;
    } else {
        local_seq += len;
    }
}

int tcp_connect(uint32_t dest_ip, uint16_t dest_port){
    if (tcp_state != TCP_CLOSED){
        return -1;
    }

    remote_ip = dest_ip;
    remote_port = dest_port;
    tcp_state = TCP_SYN_SENT;

    for (int attempt = 0; attempt < 3; attempt++){
        tcp_send_packet(NULL, 0, TCP_SYN);
        uart_puts("TCP: SYN Sent\n");

        // crude delay loop (assumes uart polling in main)
        for (volatile int i = 0; i < 1000000; i++);

        if (tcp_state == TCP_ESTABLISHED){
            uart_puts("TCP: Connection established after retry\n");
            return 0;
        }
    }

    uart_puts("TCP: Connection timeout\n");
    tcp_state = TCP_CLOSED;
    return -1;

}

int tcp_send(const char* data, int len){
    if (tcp_state != TCP_ESTABLISHED){
        return -1;
    }

    tcp_send_packet(data, len, TCP_ACK | TCP_PSH);
    uart_puts("TCP: Data sent\n");

    return len;
}

int tcp_receive(char* buf, int* len) {
    if (tcp_rx_len == 0) {
        *len = 0;
        return 0;  // No data available
    }

    *len = tcp_rx_len;
    memcpy(buf, tcp_rx_buffer, tcp_rx_len);
    tcp_rx_len = 0;  // Clear buffer after read
    return 1;        // Data returned
}

void tcp_close(void){

    if (tcp_state != TCP_ESTABLISHED){
        return;
    }

    tcp_send_packet(NULL, 0, TCP_FIN | TCP_ACK);
    tcp_state = TCP_FIN_WAIT_1;
    uart_puts("TCP: FIN sent\n");
}

void tcp_recv(const char* packet, int len, uint32_t src_ip){
    if (len < TCP_HEADER_SIZE){
        return;
    }

    const tcp_header_t* hdr = (const tcp_header_t*)packet;
    int header_len = (hdr->data_offset >> 4) * 4;
    int payload_len = len - header_len;

    uint32_t seq = ntohl(hdr->seq_num);
    // uint32_t ack = ntohl(hdr->ack_num);

    if ((hdr->flags & TCP_SYN) && is_listening && tcp_state == TCP_CLOSED){
        remote_ip = src_ip;
        remote_port = ntohs(hdr->src_port);
        remote_seq = ntohl(hdr->seq_num) + 1;
        tcp_state = TCP_ESTABLISHED;
        local_seq = 0x2000;         // server's starting seq num
        tcp_send_packet(NULL, 0, TCP_SYN | TCP_ACK);
        uart_puts("TCP: Accepted incoming connections\n");
        return;
    }

    if (hdr->flags & TCP_SYN && tcp_state == TCP_SYN_SENT){
        remote_seq = seq + 1;
        tcp_state = TCP_ESTABLISHED;
        tcp_send_packet(NULL, 0, TCP_ACK);
        uart_puts("TCP: Connection Established\n");
        return;
    }

    if (hdr->flags & TCP_ACK && tcp_state == TCP_FIN_WAIT_1){
        tcp_state = TCP_FIN_WAIT_2;
        uart_puts("TCP: ACK for FIN Redeived\n");
        return;
    }

    if (hdr->flags & TCP_FIN){
        remote_seq  += 1;
        tcp_send_packet(NULL, 0, TCP_ACK);
        tcp_state = TCP_TIME_WAIT;
        uart_puts("TCP: FIN received, sent ACK\n");
        return;
    }

    if (payload_len > 0 && tcp_state == TCP_ESTABLISHED){
        const char* payload = packet + header_len;

        uart_puts("TCP: Data Received: ");

        for (int i = 0; i < payload_len; i++){
            uart_putc(payload[i]);
        }
        uart_puts("\n");

        if (payload_len <= TCP_RX_BUFFER_SIZE) {
            memcpy(tcp_rx_buffer, payload, payload_len);
            tcp_rx_len = payload_len;
        }

        remote_seq = seq + payload_len;
        tcp_send_packet(NULL, 0, TCP_ACK);
    }
}

void tcp_listen(uint16_t port) {
    local_port = port;
    is_listening = 1;
    tcp_state = TCP_CLOSED;
    uart_puts("TCP: Listening for connections...\n");
}

// Part                         Description
// tcp_send_packet()	        Assembles and sends a TCP packet with given flags and data.
// tcp_connect()	            Starts a TCP connection with SYN, sets state.
// tcp_send()	                Sends data with PSH+ACK flag.
// tcp_close()	                Sends FIN to begin connection teardown.
// tcp_recv()	                Handles SYN-ACK handshake, data, and FIN teardown.
// tcp_state enum	            Tracks connection state. No server-side logic yet.