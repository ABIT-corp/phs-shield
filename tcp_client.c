#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/dns.h"
#include "lwip_delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "tcp_client.h"
#include "phs_shield.h"

#define    TCP_CLIENT_WORK_SIZE       256
#define    TCP_CLIENT_CONNECT_TIMEOUT 20
#define    TCP_CLIENT_CLOSE_TIMEOUT   30
#define    TCP_CLIENT_TIMEOUT         5

/* protocol states */
enum client_states {
    ES_NOT_CONNECTED = 0,
    ES_CONNECT_FAIL,
    ES_CONNECTED,
    ES_RECEIVED,
    ES_CLOSING,
    ES_CLOSED,
};

/* structure to be passed as argument to the tcp callbacks */
struct client {
    enum client_states state; /* connection status */
    struct tcp_pcb *pcb;      /* pointer on the current tcp_pcb */
    struct pbuf *p_tx;        /* pointer on pbuf to be transmitted */
    struct pbuf *p_rx;        /* pointer on pbuf to be received */
    s32_t count;
};

static struct tcp_pcb *client_pcb;
static struct client *client_es;

extern xSemaphoreHandle semaphore_tcp;
extern struct_psld psld;


/* Private function prototypes -----------------------------------------------*/
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_connection_close(struct tcp_pcb *tpcb, struct client * es);
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_client_send(struct tcp_pcb *tpcb, struct client * es);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);


/* Private functions ---------------------------------------------------------*/
volatile int dns_found_flag;

/**
 * Check retry count
 * @param  mode: select parameter that count up
 *         count_enable: count up or not
 * @return None
 */
void tcp_client_retry_count(u32_t mode, u32_t count_enable)
{
    switch(mode){
    case SYSRESET_CONNECTRETRY:
        if(count_enable == 0){
            psld.tcpip_connect_retry = 0;
            return;
        }
        if(psld.tcpip_connect_retry++>=TCPIP_CONNECT_PRETRY_TIMEOUT){
            rtc_backup_write(19, mode);
            printf_queued_flush();
            NVIC_SystemReset();
        }
    break;
    case SYSRESET_BUFALLOCRETRY:
        if(count_enable == 0){
            psld.buf_alloc_retry = 0;
            return;
        }
        if(psld.buf_alloc_retry++>=BUF_ALLOC_RETRY_TIMEOUT){
            rtc_backup_write(19, mode);
            printf_queued_flush();
            NVIC_SystemReset();
        }
    break;
    case SYSRESET_TCPIPBUSY:
        if(count_enable == 0){
            psld.tcpip_busy_retry = 0;
            return;
        }
        if(psld.tcpip_busy_retry++>=TCPIP_BUSY_TIMEOUT){
            rtc_backup_write(19, mode);
            printf_queued_flush();
            NVIC_SystemReset();
        }
    break;
    }
}

static void tcp_client_dns_callback(void *arg)
{
    dns_found_flag = 1;
}

/**
 * Connects to the TCP server
 * @param  None
 * @return None
 */
s32_t tcp_client_connect(u8_t *server, u16_t port_number) {
    u32_t status, timeout_count;
    u8_t number[4];
    struct ip_addr ip4_address;

    printf("TCP/IP: open request\n");
    /* check modem status */
    if((status = modem_ppp_link_check())>0){
        return status+10;
    }

    /* check tcpip is in use */
    status = tcpip_tcp_busy();
    if(status){
        return status;
    }

    /* create new tcp pcb */
    client_pcb = tcp_new();
    printf("TCP/IP: tcp_new()=%08x\n", (unsigned int)client_pcb);
    if (client_pcb == NULL) {
        vTaskDelay(1000);
        client_pcb = tcp_new();
        if(client_pcb){
            printf("TCP/IP: tcp_new2()=%08x\n", (unsigned int)client_pcb);
        }else{
            /* deallocate the pcb */
            memp_free(MEMP_TCP_PCB, client_pcb);
            printf("TCP/IP: can not create tcp pcb\n");
            tcp_client_retry_count(SYSRESET_BUFALLOCRETRY, 1);
            return 1;
        }
    }

    /* allocate structure es to maintain tcp connection informations */
    client_es = (struct client *) mem_malloc(sizeof(struct client));
    printf("TCP/IP: mem_malloc()=%08x\n", (unsigned int)client_es);
    if (client_es == NULL) {
        /* deallocate the pcb */
        memp_free(MEMP_TCP_PCB, client_pcb);
        /* deallocate the es */
        mem_free(client_es);
        printf("TCP/IP: can not create es\n");
        tcp_client_retry_count(SYSRESET_BUFALLOCRETRY, 1);
        return 2;
    }
    tcp_client_retry_count(SYSRESET_BUFALLOCRETRY, 0);

    if(ip4_addresse_atoi(server, number)!=0){
        dns_found_flag = 0;
        /* query DSN server */
        status = dns_gethostbyname((const char *)server, &ip4_address, (dns_found_callback)tcp_client_dns_callback, 0);
        timeout_count = 0;
        if(status != ERR_OK){
            while(dns_found_flag == 0){
                p_mdelay(10);
                if(timeout_count++>TCP_CLIENT_CONNECT_TIMEOUT*100){
                    break;
                }
            }
            if(timeout_count>TCP_CLIENT_CONNECT_TIMEOUT*100){
                return 3;
            }
            /* IP address is already in the table */
            status = dns_gethostbyname((const char *)server, &ip4_address, (dns_found_callback)tcp_client_dns_callback, 0);
            if(status != ERR_OK){
                return 4;
            }
        }
        number[0] = (ip4_address.addr     & 0xff);
        number[1] = (ip4_address.addr>> 8 & 0xff);
        number[2] = (ip4_address.addr>>16 & 0xff);
        number[3] = (ip4_address.addr>>24 & 0xff);
    }else{
        /* server is number of IP address */
        IP4_ADDR(&ip4_address, number[0], number[1], number[2], number[3]);
    }
    printf("TCP/IP: dns is resolved [%d.%d.%d.%d]\n", number[0], number[1], number[2], number[3]);

    /* connect to destination address/port */
    client_es->state = ES_NOT_CONNECTED;
    status = tcp_connect(client_pcb, &ip4_address, port_number, tcp_client_connected);
    printf("TCP/IP: tcp_connect\n");
    if (status != ERR_OK) {
        status = 5;
        tcp_client_retry_count(SYSRESET_CONNECTRETRY, 1);
    }else{
        printf("TCP/IP: connect wait\n");
        /* wait connection */
        timeout_count = 0;
        while(client_es->state == ES_NOT_CONNECTED){
            if(timeout_count++>TCP_CLIENT_CONNECT_TIMEOUT*100){
                status = 6;
                break;
            }
            p_mdelay(10);
        }
        if(client_es->state != ES_CONNECTED){
            status =  7;
            tcp_client_retry_count(SYSRESET_CONNECTRETRY, 1);
        }
    }

    if(status){
        printf("TCP/IP: open request error[%d]\n", (int)status);
        tcp_close(client_pcb);
        mem_free(client_es);
        client_es = NULL;
        return status;
    }
    tcp_client_retry_count(SYSRESET_CONNECTRETRY, 0);
    return 0;
}


/**
 * Function called when TCP connection established
 * @param tpcb: pointer on the connection contol block
 * @param err: when connection correctly established err should be ERR_OK 
 * @return err_t: returned error
 */
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    if (client_es == NULL) {
        tcp_client_connection_close(tpcb, client_es);
        err = ERR_MEM;
    }
    if (err == ERR_OK) {
        printf("TCP/IP: connected\n");

        client_es->pcb = tpcb;
        client_es->p_tx = 0;
        client_es->count = 0;

        /* pass newly allocated es structure as argument to tpcb */
        tcp_arg(tpcb, client_es);
        tcp_recv(tpcb, tcp_client_recv);
        tcp_sent(tpcb, tcp_client_sent);
        tcp_poll(tpcb, tcp_client_poll, 1);

        client_es->state = ES_CONNECTED;
        err = ERR_OK;
    } else {
        tcp_client_connection_close(tpcb, client_es);
        client_es->state = ES_CONNECT_FAIL;
    }
    xSemaphoreGive(semaphore_tcp);
    return err;
}

/**
 * tcp_receiv callback
 * @param arg: argument to be passed to receive callback 
 * @param tpcb: tcp connection control block 
 * @param err: receive error code 
 * @return err_t: retuned error
 */
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    struct client *es;
    err_t ret_err;

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    printf("TCP/IP: received ");
    LWIP_ASSERT("arg != NULL", arg != NULL);

    es = (struct client *)arg;

    /* if we receive an empty TCP frame from server => close connection */
    if (p == NULL) {
        printf("empty frame for remote host closed\n");
        es->state = ES_CLOSING;
        if (es->p_tx == NULL) {
            /* we're done sending, close connection */
            tcp_client_connection_close(tpcb, es);
        } else {
            /* send remaining data*/
            tcp_client_send(tpcb, es);
        }
        ret_err = ERR_OK;
    }else if (err != ERR_OK) {
        /* free received pbuf*/
        printf("non empty frame for some err\n");
        if (p != NULL) {
            pbuf_free(p);
        }
        ret_err = err;
    }else if (es->state == ES_CONNECTED || es->state == ES_RECEIVED) {
        /* increment message count */
        printf("normally\n");
        es->count = p->tot_len;

        if(client_es->p_rx==NULL){
            client_es->p_rx = p;
        }else{
            struct pbuf *ptr;
            ptr = client_es->p_rx;

            while(1){
                ptr->tot_len += p->tot_len;
                if(ptr->next==NULL){
                    break;
                }else{
                    ptr = ptr->next;
                }
            }
            ptr->next = p;
        }
        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);

        es->state = ES_RECEIVED;
        ret_err = ERR_OK;
    }else {
        printf("when connection already closed\n");
        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);

        /* free pbuf and do nothing */
        pbuf_free(p);
        ret_err = ERR_OK;
    }
    xSemaphoreGive(semaphore_tcp);
    return ret_err;
}

/**
 * function used to send data
 * @param  tpcb: tcp control block
 * @param  es: pointer on structure of type client containing info on data
 *             to be sent
 * @return None
 */
static err_t tcp_client_send(struct tcp_pcb *tpcb, struct client * es) {
    struct pbuf *ptr;
    err_t wr_err = ERR_OK;

    printf("TCP/IP: send\n");
    while ((wr_err == ERR_OK) && (es->p_tx != NULL) && (es->p_tx->len <= tcp_sndbuf(tpcb))) {
        /* get pointer on pbuf from es structure */
        ptr = es->p_tx;

        /* enqueue data for transmission */
        wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

        if (wr_err == ERR_OK) {
            /* continue with next pbuf in chain (if any) */
            if(ptr->next != NULL){
                es->p_tx = ptr->next;
            }else{
                es->p_tx = NULL;
            }
            if (es->p_tx != NULL) {
                /* increment reference count for es->p */
                pbuf_ref(es->p_tx);
            }

            /* free pbuf: will free pbufs up to es->p  */
            pbuf_free(ptr);
        } else if (wr_err == ERR_MEM) {
            /* we are low on memory, try later, defer to poll */
            printf("TCP/IP: send() memory error\n");
            es->p_tx = ptr;
        }else{
            return 1;
        }
    }
    return 0;
}

/**
 * This function implements the tcp_poll callback function
 * @param  arg: pointer on argument passed to callback
 * @param  tpcb: tcp connection control block
 * @return err_t: error code
 */
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    err_t ret_err;
    struct client *es;

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    printf("TCP/IP: polled\n");
    if(tpcb == NULL){
        return ERR_ABRT;
    }
    es = (struct client*) arg;
    if (es != NULL) {
        if (es->p_tx != NULL) {
            /* there is a remaining pbuf (chain) , try to send data */
            tcp_client_send(tpcb, es);
        } else {
            /* no remaining pbuf (chain)  */
            if (es->state == ES_CLOSING) {
                /* close tcp connection */
                tcp_client_connection_close(tpcb, es);
            }
        }
        ret_err = ERR_OK;
    } else {
        /* nothing to be done */
        tcp_abort(tpcb);
        ret_err = ERR_ABRT;
    }
    xSemaphoreGive(semaphore_tcp);
    return ret_err;
}

/**
 * This function implements the tcp_sent LwIP callback (called when ACK
 *   is received from remote host for sent data)
 * @param  arg: pointer on argument passed to callback
 * @param  tcp_pcb: tcp connection control block
 * @param  len: length of data sent 
 * @return err_t: returned error code
 */
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct client *es;

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    LWIP_UNUSED_ARG(len);

    es = (struct client *) arg;

    if (es->p_tx != NULL) {
        /* still got pbufs to send */
        tcp_client_send(tpcb, es);
    }
    xSemaphoreGive(semaphore_tcp);

    return ERR_OK;
}

/*
 * This function is used to close the tcp connection with server
 * @param tpcb: tcp connection control block
 * @param es: pointer on client structure
 * @return None
 */
static void tcp_client_connection_close(struct tcp_pcb *tpcb, struct client * es) {
    err_t err;
    printf("TCP/IP: disconnected\n");

    /* remove callbacks */
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);

    /* close tcp connection */
    err = tcp_close(tpcb);
    if(err){
        printf("TCP/IP: tcp_close[0x%08x] error: %d\n", (int)err, (int)tpcb);
    }
    if(client_es->p_tx!=NULL){
        pbuf_free(client_es->p_tx);
    }
    es->state = ES_CLOSED;
}



/*
 *
 *  ARDUINO Command API TCPIP Access Tools
 *
 */
/*
 * Convert number array from IP address string [xxx.xxx.xxx.xxx]
 * @server: pointer on IP address string
 * @number: pointer on converted number array
 * @return 0:success 1:not number but server name
 */
s32_t ip4_addresse_atoi(u8_t *server, u8_t *number)
{
    u32_t i, bit, string_length;
    u8_t data, ip_index, number_string[4];

    string_length = strlen((char *)server)+1;

    i = 0;
    bit = 0;
    ip_index = 0;
    while(i<string_length && ip_index<4){
        data = server[i++];
        if(data == '.'||data == '\0'){
            number_string[bit] = '\0';
            bit = 0;
            number[ip_index++] = atoi((char *)number_string);
        }else{
            if(!isdigit(data)){
                return 1;
            }
            number_string[bit++] = data;
        }
    }
    return 0;
}

/*
 * Check if tcp/ip is busy
 * @return none
 */
s32_t tcpip_tcp_busy()
{
    extern struct_psld psld;

    if(client_es == NULL){
        tcp_client_retry_count(SYSRESET_TCPIPBUSY, 0);
        return 0;
    }
    if(client_es->state == ES_CONNECTED || client_es->state == ES_RECEIVED || client_es->state == ES_CLOSING){
        printf("TCP/IP: busy\n");
        tcp_client_retry_count(SYSRESET_TCPIPBUSY, 1);
        return 309;
    }
    tcp_client_retry_count(SYSRESET_TCPIPBUSY, 0);
    return 0;
}

void tcp_status()
{
#if 0
    int i;
    rtc_time_display(0);
    MEM_STATS_DISPLAY();
    SYS_STATS_DISPLAY();
    for(i=0;i<5;i++){
        MEMP_STATS_DISPLAY(i);
    }
    //tcp_debug_print_pcbs();
#endif
}

/*
 * Request TCP Disconnect
 * @return none
 */
s32_t tcpip_tcp_disconnect()
{
    u32_t timeout_count, status;
    extern void status_task_list_view();

    if (client_es == NULL) {
        return 615;
    }

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    printf("TCP/IP: close request\n");

    if(client_es->state == ES_CLOSED){
        if (client_es != NULL) {
            mem_free(client_es);
            client_es = NULL;
        }
        tcp_status();
        xSemaphoreGive(semaphore_tcp);
        return 0;
    }else if(client_es->state != ES_CLOSING){
        client_es->state = ES_CLOSING;
        xSemaphoreGive(semaphore_tcp);
    }

    status = 0;
    timeout_count = 0;
    while(client_es->state != ES_CLOSED){
        p_mdelay(10);
        if(timeout_count++>TCP_CLIENT_CLOSE_TIMEOUT*100){
            printf("TCP/IP: close timeout\n");
            tcp_client_connection_close(client_pcb, client_es);
            //status = 614;
            //break;
        }
    }

    if (client_es != NULL) {
        mem_free(client_es);
        client_es = NULL;
    }
    tcp_status();
    return status;
}


/*
 * Request TCP Write
 * @source_buffer: pointer on buffer address to be transferred
 * @source_size: size of transfer data
 * @return 0:success 1:error
 */
s32_t tcpip_tcp_write(u8_t *source_buffer, size_t source_size)
{
    struct pbuf *ptr;
    s8_t status;

    printf("TCP/IP: write request\n");
    status = 0;
    ptr = pbuf_alloc(PBUF_TRANSPORT, source_size, PBUF_POOL);
    if(ptr==0){
        printf("TCP/IP: pbuf_alloc() error\n");
        tcp_client_retry_count(SYSRESET_BUFALLOCRETRY, 1);
        return 1;
    }
    tcp_client_retry_count(SYSRESET_BUFALLOCRETRY, 0);
    pbuf_take(ptr, (u8_t *)source_buffer, source_size);

    /* add chain buffer tree */
    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    if(client_es->p_tx == NULL){
        client_es->p_tx = ptr;
    }else{
        client_es->p_tx->next = ptr;
    }
    xSemaphoreGive(semaphore_tcp);

    return status;
}


/*
 * Request TCP Read
 * @buffer: Pointer(address type) on buffer address to be transferred
 * @size: Pointer on size of transfer data
 * @timeout_sec: time out[second]
 * @return 0:success 1:error
 */
s32_t tcpip_tcp_read(u8_t **buffer, u16_t *size, size_t timeout_sec)
{
    struct pbuf *ptr;
    u32_t timeout_count, status, request_byte;

    printf("TCP/IP: read request\n");
    request_byte = *size;

    if(client_es->p_rx == NULL){
        timeout_count = 0;
        while(1){
            if((client_es->state == ES_RECEIVED)||
               (client_es->state == ES_CLOSING)||
               (timeout_count++>TCP_CLIENT_TIMEOUT*100)){
                break;
            }
            p_mdelay(10);
        }
        if(timeout_count>=TCP_CLIENT_TIMEOUT*100){
            printf("TCP/IP: read timeout\n");
            return 1;
        }
    }

    xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
    ptr = client_es->p_rx;
    timeout_count = 0;
    while(1){
        if(ptr->len || (timeout_count++>TCP_CLIENT_TIMEOUT*100)){
            break;
        }else if(ptr->next){
            ptr = ptr->next;
        }else if(ptr->next==0){
            printf("TCP/IP: read list error\n");
            xSemaphoreGive(semaphore_tcp);
            return 2;
        }
        p_mdelay(10);
    }
    if(timeout_count>=TCP_CLIENT_TIMEOUT*100){
        printf("TCP/IP: read list timeout\n");
        xSemaphoreGive(semaphore_tcp);
        return 2;
    }

    if(ptr->payload==0 || ptr->tot_len==0){
        status = 1;
    }else if(client_es->state == ES_CLOSING && ptr->tot_len == 0){
        status = 635;
    }else{
        status = 0;
    }

    if(status==0){
        *buffer = ptr->payload;
        if(request_byte < ptr->len){
            *size = request_byte;
            ptr->tot_len -= request_byte;
            ptr->len -= request_byte;
            ptr->payload += request_byte;
            client_es->p_rx->tot_len = ptr->tot_len;
        }else{
            *size = ptr->len;
            ptr->tot_len -= ptr->len;
            ptr->len = 0;
            client_es->p_rx->tot_len = ptr->tot_len;
        }
    }
    xSemaphoreGive(semaphore_tcp);

    return status;
}


void tcpip_tcp_read_buffer_free(uint32_t force_free)
{
    struct pbuf *ptr, *next;

    ptr = client_es->p_rx;

    if(ptr==NULL){
        return;
    }
    if(ptr->tot_len == 0 || (force_free && ptr->tot_len) ){
        xSemaphoreTake(semaphore_tcp, portMAX_DELAY);
        pbuf_free(ptr);
        client_es->p_rx = NULL;
        client_es->state = ES_CONNECTED;
        xSemaphoreGive(semaphore_tcp);
    }
}


/*
 * Request HTTP GET
 * @server: server name string
 * @port_number: Port number
 * @header: http header string
 * @path: http path address string
 * @ssl: 0:http 1:https(no supported)
 * @return 0:success 1:error
 */
s32_t tcpip_tcp_httpget(u8_t *server, u16_t port_number, u8_t *path, u8_t *header, u8_t ssl)
{
    s32_t status, size;
    char *request;

    status = tcpip_tcp_busy();
    if(status){
        return status;
    }
    status = tcp_client_connect(server, port_number);
    if(status){
        return status;
    }

    size = strlen((char *)server) + strlen((char *)path) + strlen((char *)header) + 1;
    request = (char *)malloc(size);

    sprintf((char *)request, "GET /%s HTTP/1.1\r\n", path);
    strcat(request, (char *)header);
    strcat(request, "\r\n");
    strcat(request, "Host: ");
    strcat(request, (char *)server);
    strcat(request, "\r\n\r\n");
    status = tcpip_tcp_write((u8_t *)request, strlen((char *)request));

    free(request);

    return status;
}


/*
 * Request HTTP POST
 * @server: server name string
 * @port_number: Port number
 * @header: http header string
 * @path: http path address string
 * @path: http body string
 * @ssl: 0:http 1:https(no supported)
 * @return 0:success 1:error
 */
s32_t tcpip_tcp_httppost(u8_t *server, u16_t port_number, u8_t *path, u8_t *header, u8_t *body, u8_t ssl)
{
    s32_t status, size;
    char *request;

    status = tcpip_tcp_busy();
    if(status){
        return status;
    }
    status = tcp_client_connect(server, port_number);
    if(status){
        return status;
    }

    size = strlen((char *)server) + strlen((char *)path) + strlen((char *)header) + strlen((char *)body) + 1;
    request = (char *)malloc(size);

    sprintf((char *)request, "POST /%s HTTP/1.1\r\n", path);
    strcat(request, (char *)header);
    strcat(request, "\r\n");
    strcat(request, "Host: ");
    strcat(request, (char *)server);
    strcat(request, "\r\n\r\n");
    strcat(request, (char *)body);
    strcat(request, "\r\n");
    status = tcpip_tcp_write((u8_t *)request, strlen((char *)request));

    free(request);

    return status;
}


