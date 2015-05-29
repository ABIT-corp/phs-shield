#ifndef LWIP_PPP_H
#define LWIP_PPP_H

void lwip_main_restart();
void lwip_main_close(uint8_t);
void lwip_main(void);
static void tcpip_init_done(void *);
static void ppp_rx_thread(void *);
u32_t sio_write(sio_fd_t fd, uint8_t *data, u32_t length);
u32_t sio_read(sio_fd_t fd, uint8_t *data, u32_t length);
static void ppp_notify_phase_cb(ppp_pcb *pcb, u8_t phase, void *ctx);
static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx);


#endif
