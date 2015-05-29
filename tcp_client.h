#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H



int32_t ip4_addresse_atoi(uint8_t *server, uint8_t *number);
int32_t tcp_client_connect(uint8_t *server, uint16_t port_number);
int32_t tcpip_tcp_disconnect();
int32_t tcpip_tcp_write(uint8_t *source_buffer, size_t source_size);
int32_t tcpip_tcp_read(uint8_t **buffer, uint16_t *size, size_t timeout_sec);
void tcpip_tcp_read_buffer_free();
int32_t tcpip_tcp_httpget(uint8_t *server, uint16_t port_number, uint8_t *path, uint8_t *header, uint8_t ssl);
int32_t tcpip_tcp_httppost(uint8_t *server, uint16_t port_number, uint8_t *path, uint8_t *header, uint8_t *body, uint8_t ssl);
int32_t tcpip_tcp_busy();
int32_t modem_ppp_link_check();


#endif
