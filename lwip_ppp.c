#ifdef __linux__
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#endif /* __linux___ */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#ifdef __linux__
#include <sysexits.h>
#endif

/* lwIP stuff */
#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/tcp_impl.h"
#include "lwip/inet_chksum.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/ppp/ppp.h"
#include "lwip/ip_addr.h"
#include "arch/perf.h"
#include "lwip/icmp.h"
#include "lwip/raw.h"
#include "lwip/api.h"

#include "phs_shield.h"
#include "serial_posix_api.h"
#include "lwip_delay.h"
#include "modem.h"
#include "lwip_ppp.h"


static ppp_pcb *ppps;
static sio_fd_t sio_fd;

extern struct_psld psld;
extern xSemaphoreHandle semaphore_puts;


void
lwip_main_restart()
{
    psld.ppp_restart = 1;
}

void
lwip_main_close(uint8_t non_block)
{
    if(psld.ppp_enable!=1){
        return;
    }

    modem_dsr(1);
    p_mdelay(10);
    modem_escape(psld.modemobj);
    p_mdelay(10);
    modem_dsr(0);

    psld.ppp_enable = 0;

    if(non_block){
        return;
    }
    while(psld.modem_status == PPP_LINK);
}

void
lwip_main(void)
{
    sys_sem_t sem;

    while(psld.modem_status != PPP_CONNECT){
        p_mdelay(500);
    }
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    printf("System initialized.\n");
    xSemaphoreGive(semaphore_puts);

    if (sys_sem_new(&sem, 0) != ERR_OK) {
        LWIP_ASSERT("Failed to create semaphore", 0);
    }

    /*
     * Initialize the lwIP stack.
     * The call to tcpip_init() will cause the stack to spawn its
     * dedicated thread: tcpip_thread().
     * Upon completion, callback function tcpip_init_done() will be called
     * in tcpip_thread context.
     */
    tcpip_init(tcpip_init_done, &sem);
    sys_sem_wait(&sem);
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    printf("PPP/IP initialized.\n");
    xSemaphoreGive(semaphore_puts);

    /* reuse sem */
    psld.ppp_done = 0;
    psld.ppp_enable = 1;
    sys_thread_new("ppp_rx", ppp_rx_thread, &sem, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);

    while(1){
        psld.ppp_restart = 0;
        printf("PPP started.\n");

        pppapi_open(ppps, 0);
        sys_sem_signal(&sem);

        while(psld.ppp_done==0);
        if(psld.modem_status == PPP_LINK){
            extern xQueueHandle queue_command;
            extern void rtc_time_synchronize();
            int32_t function;

            xSemaphoreTake(semaphore_puts, portMAX_DELAY);
            printf("PPP Link is established.\n");
            xSemaphoreGive(semaphore_puts);

            function = (int32_t)rtc_time_synchronize;
            xQueueSendToBack(queue_command, &function, 0);
        }

        while(psld.modem_status == PPP_LINK) {
            p_mdelay(500);
        }

        psld.ppp_enable = 0;
        sys_sem_signal(&sem);
        pppapi_close(ppps);
        printf("PPP Link is down.\n");

        while(psld.ppp_restart==0){
            p_mdelay(500);
        }
        psld.ppp_done = 0;
        psld.ppp_enable = 1;
    }
}


/*
 * called from tcpip_thread context on completion of tcpip_init()
 */
static void
tcpip_init_done(void *arg)
{
    sys_sem_t *sem;
    sem = (sys_sem_t *) arg;

    /*
     * initialize PPP stack
     */
    ppps = ppp_new();
    ppp_set_default(ppps);
    ppp_set_auth(ppps, PPPAUTHTYPE_PAP, "prin", "prin");
    ppp_over_serial_create(ppps, sio_fd, ppp_link_status_cb, NULL);
    ppp_set_notify_phase_callback(ppps, ppp_notify_phase_cb);

    sys_sem_signal(sem);
}

static void
ppp_rx_thread(void *arg)
{
    sys_sem_t *sem;
    u8_t buffer[128];
    int len;

    memset(buffer, 0, sizeof(buffer));
    sem = (sys_sem_t *) arg;

    while (1) {
        sys_sem_wait(sem);
        if (psld.ppp_enable) {
            while (1) {
                if (!psld.ppp_enable) {
                    break;
                }
                len = sio_read(0, buffer, 128);
                if (len < 0) {
                    pppapi_sighup(ppps);
                } else {
                    pppos_input(ppps, buffer, len); // pppos_input() has to be regularly accessed, even if no data
                }
            }
        }
    }
}

static void
ppp_notify_phase_cb(ppp_pcb *pcb, u8_t phase, void *ctx)
{
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    printf("0x%02x\n", phase);
    xSemaphoreGive(semaphore_puts);
}


static void
ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    switch (err_code) {
    case PPPERR_NONE: /* No error. */
    {
        struct ppp_addrs *ppp_addrs = &pcb->addrs;

        printf("pppLinkStatusCallback: PPPERR_NONE");
        printf(" our_ipaddr=%s", inet_ntoa(ppp_addrs->our_ipaddr.addr));
        printf(" his_ipaddr=%s", inet_ntoa(ppp_addrs->his_ipaddr.addr));
        printf(" netmask=%s", inet_ntoa(ppp_addrs->netmask.addr));
        printf(" dns1=%s", inet_ntoa(ppp_addrs->dns1.addr));
        printf(" dns2=%s\n", inet_ntoa(ppp_addrs->dns2.addr));
        psld.modem_status = PPP_LINK;
    }
        break;

    case PPPERR_PARAM: /* Invalid parameter. */
        printf("pppLinkStatusCallback: PPPERR_PARAM\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_OPEN: /* Unable to open PPP session. */
        printf("pppLinkStatusCallback: PPPERR_OPEN\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_DEVICE: /* Invalid I/O device for PPP. */
        printf("pppLinkStatusCallback: PPPERR_DEVICE\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_ALLOC: /* Unable to allocate resources. */
        printf("pppLinkStatusCallback: PPPERR_ALLOC\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_USER: /* User interrupt. */
        printf("pppLinkStatusCallback: PPPERR_USER\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_CONNECT: /* Connection lost. */
        printf("pppLinkStatusCallback: PPPERR_CONNECT\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_AUTHFAIL: /* Failed authentication challenge. */
        printf("pppLinkStatusCallback: PPPERR_AUTHFAIL\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    case PPPERR_PROTOCOL: /* Failed to meet protocol. */
        printf("pppLinkStatusCallback: PPPERR_PROTOCOL\n");
        psld.modem_status = PPP_NO_LINK;
        break;

    default:
        printf("pppLinkStatusCallback: unknown errCode %d\n", err_code);
        psld.modem_status = PPP_NO_LINK;
        break;
    }
    psld.ppp_done = 1;
    xSemaphoreGive(semaphore_puts);
}


/*
 * call from ppp.c
 */
u32_t
sio_write(sio_fd_t fd, uint8_t *data, u32_t length)
{
    int result;

    result = modem_write((uint8_t *) data, (size_t) length);
#if 0
    printf("WRITE\n");
    hexdump(data, len);
#endif
    return (result);
}


u32_t
sio_read(sio_fd_t fd, uint8_t *data, u32_t length)
{
    int result;

    result = modem_read((uint8_t *) data, (size_t) length);
#if 0
    if(result){
        printf("READ\n");
        hexdump(data, len);
    }
#endif
    return (result);
}

