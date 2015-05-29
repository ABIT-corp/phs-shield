/* 
 * File:   modem.h
 * Author: abit
 *
 * Created on 2014/12/02, 11:05
 */

#ifndef MODEM_H
#define MODEM_H

#define RX_BUFFER_SIZE 256
#define MAX_RESULT_LINES 32

typedef enum {
    RES_UNKNOWN,
    RES_OK,
    RES_ERROR,
    RES_CONNECT,
    RES_NO_CARRIER,
    RES_DELAYED,
    RES_BUSY,
    RES_RING,
    RES_PROG,
    RES_ALERT
} result_type;



#define    MODEM_ESCAPE           "+++"
#define    MODEM_ONHOOK           "ATH"
#define    MODEM_RESET            "ATZ"
#define    MODEM_CALL             "ATD"
#define    MODEM_MAILCALL         "ATDI"
#define    MODEM_ON_HOOK          "ATH0"
#define    MODEM_ID               "ATI6"
#define    MODEM_COMMAND_ECHO_ON  "ATE1"
#define    MODEM_COMMAND_ECHO_OFF "ATE0"
#define    MODEM_RESULT_ON        "ATQ1"
#define    MODEM_RESULT_OFF       "ATQ0"
#define    MODEM_CALLNUMBER_ON    "AT#B1"
#define    MODEM_CALLNUMBER_OFF   "AT#B0"
#define    MODEM_CALLRESULT_ON    "AT#S1"
#define    MODEM_CALLRESULT_OFF   "AT#S0"
#define    MODEM_MAIL_RECEIVE_ON  "AT#P1"
#define    MODEM_MAIL_RECEIVE_OFF "AT#P0"
#define    MODEM_MAIL_DATA_RX     "ATS211?"
#define    MODEM_MAIL_DATA_TX     "ATS202"
#define    MODEM_CALL_DATALINK    "ATD0570570711##64"
#define    MODEM_RSSI             "AT#Q5"
#define    MODEM_CSIDRSSI         "AT@K1"
#define    MODEM_IMEI             "ATI6"
#define    MODEM_CSID_ON          "AT@LBC1"
#define    MODEM_CSID             "AT@LBC?"

#define    RESULT_OK              "OK"
#define    RESULT_ATE0OK          "ATE0OK"
#define    RESULT_CONNECT         "CONNECT"
#define    RESULT_RING            "RING"
#define    RESULT_ID              "ID="
#define    RESULT_NO_CARRIER      "NO CARRIER"
#define    RESULT_ERROR           "ERROR"
#define    RESULT_BUSY            "BUSY"
#define    RESULT_DELAYED         "DELAYED"
#define    RESULT_PROG            "PROG"
#define    RESULT_ALERT           "ALERT"
#define    RESULT_CONNECT         "CONNECT"
#define    RESULT_DISC            "DISC"
#define    RESULT_NO_CARRIER      "NO CARRIER"

#define    LIGHTMAIL_MAX_SIZE     (3*90+21)
#define    LIGHTMAIL_ID_SIZE      (21)
#define    LIGHTMAIL_CODE         "128145000013"

#define MODEM_LINKUP_TIMEOUT    30

typedef struct
{
    uint8_t  flag;
    int8_t   id[12];
    int8_t   *buffer;
} struct_lightmail;



modem * modem_create(void);
void modem_destroy(modem *obj);
int modem_attach(modem *obj, serial *device);
int modem_initialize(modem *obj);
int modem_command(modem *obj, const char *command);
result_type modem_get_result_type(modem *obj);
const char * const * modem_get_result_str(modem *obj);
int modem_rawmode(modem *obj);
int modem_escape(modem *obj);
uint32_t modem_write(uint8_t *, size_t);
uint32_t modem_read(uint8_t *, size_t);
int32_t modem_powerup(void);
int32_t modem_mail_write(uint8_t *, uint8_t *);
int32_t modem_mail_read(uint8_t *, size_t);
int32_t modem_mail_get(int8_t **, int8_t **);
int32_t modem_check_ringcall(void);
void modem_check_ringcall_interrupt(void);
int32_t modem_mail_available(void);
int32_t modem_rssi_get(uint8_t *rssi);
int32_t modem_imei_get(uint8_t *imei);
int32_t modem_location_get(uint8_t*);
void profile_update();
void modem_mode_change();
void modem_flag_initialize();
int32_t modem_setup(serial *serialobj);
void modem_ppp_linkup_surveillance();
uint32_t modem_ppp_link_reconnect();

#endif    /* MODEM */

