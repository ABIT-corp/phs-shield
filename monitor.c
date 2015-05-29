/*------------------------------------------------------------------------*/
/* Universal string handler for user console interface                    */
/*------------------------------------------------------------------------*/
/*
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "phs_shield.h"
#include "rtc.h"
#include "phs_shield_uart.h"
#include "monitor.h"
#include "rtos_status.h"


#define    _CR_CRLF        1    /* 1: Convert \n ==> \r\n in the output char */
#define    _LINE_ECHO      1    /* 1: Echo back input chars in get_line function */

unsigned char (*xfunc_in)(void);    /* Pointer to the input stream */
void (*xfunc_out)(unsigned char);   /* Pointer to the output stream */
static char Line[128];
static char sprint_buffer[32];

extern struct_psld psld;



/*
 * Monitor configuration
 * @param : none
 * @return: none
 */
void monitor_configuration()
{
    xfunc_in = (void *)uart1_getchar;
    xfunc_out = (void *)uart1_putchar;
}


/*
 * Check if address can be accessed safely
 * @param : address
 * @return: [0:valid, 1:invalid]
 */
int monitor_address_check(unsigned int address)
{
    int size, i;

    size = psld.memory_map_size;

    for(i=0;i<size;i++)
    {
        if(psld.memory_map[i].start <= address && address <= psld.memory_map[i].end){
            break;
        }
    }
    if(i==size){
        puts_direct(" -->invalid address\r\n");
        return 1;
    }
    return 0;
}


/*
 * Dump a block of byte array
 * @param : addr [Heading address value]
 * @param : len  [Number of bytes to be dumped]
 * @param : mode [1:byte, 2:2bytes, 4:4bytes]
 * @return: none
 */
void put_dump (
    unsigned long addr,
    int len    ,
    int mode)
{
    int i;
    unsigned char c;
    unsigned int *pl = (unsigned int *)addr;
    unsigned short *pw = (unsigned short *)addr;
    unsigned char *pb = (unsigned char *)addr;

    sprintf(sprint_buffer, "%08x ", (unsigned int)addr);        /* address */
    puts_direct(sprint_buffer);

    for (i = 0; i < len; i++){    /* data (hexdecimal) */
        if(mode==4){
            sprintf(sprint_buffer, " %08X", pl[i]);
            puts_direct(sprint_buffer);
        }else if(mode==2){
            sprintf(sprint_buffer, " %04X", pw[i]);
            puts_direct(sprint_buffer);
        }else{
            sprintf(sprint_buffer, " %02X", pb[i]);
            puts_direct(sprint_buffer);
        }
    }

    xfunc_out(' ');

    for (i = 0; i < len; i++){    /* data (ascii) */
        if(mode==4){
            c = (pl[i] >>  0) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
            c = (pl[i] >>  8) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
            c = (pl[i] >> 16) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
            c = (pl[i] >> 24) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
        }else if(mode==2){
            c = (pw[i] >>  0) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
            c = (pw[i] >>  8) & 0xff;
            xfunc_out((c >= ' ' && c <= '~') ? c : '.');
        }else{
            xfunc_out((pb[i] >= ' ' && pb[i] <= '~') ? pb[i] : '.');
        }
    }

    puts_direct("\r\n");
}



/*
 * Get a value of the string
 * @param : str  [Pointer to pointer to the string]
 * @param : res  [Pointer to the valiable to store the value]
 * @return: [0:invalid, 1:valid ]
 */
int xatoi (
    char **str,
    unsigned long *res)
{
    unsigned long val;
    unsigned char c, r, s = 0;

    *res = 0;

    while ((c = **str) == ' ') (*str)++;    /* Skip leading spaces */

    if (c == '-') {        /* negative? */
        s = 1;
        c = *(++(*str));
    }

    if (c == '0') {
        c = *(++(*str));
        switch (c) {
        case 'x':            /* hexdecimal */
            r = 16; c = *(++(*str));
            break;
        case 'b':            /* binary */
            r = 2; c = *(++(*str));
            break;
        default:
            if (c <= ' '){
                return 1;    /* single zero */
            }
            if (c < '0' || c > '9') {
                return 0;    /* invalid char */
            }
            r = 8;            /* octal */
        }
    } else {
        if (c < '0' || c > '9'){
            return 0;        /* EOL or invalid char */
        }
        r = 10;                /* decimal */
    }

    val = 0;
    while (c > ' ') {
        if (c >= 'a'){
            c -= 0x20;
        }
        c -= '0';
        if (c >= 17) {
            c -= 7;
            if (c <= 9){
                return 0;    /* invalid char */
            }
        }
        if (c >= r) {
            return 0;        /* invalid char for current radix */
        }
        val = val * r + c;
        c = *(++(*str));
    }
    if (s) {
        val = 0 - val;        /* apply sign if needed */
    }

    *res = val;
    return 1;
}


/*
 * Get a line from the input
 * @param : buff [Pointer to the buffer]
 * @param : len  [Buffer length]
 * @return: number of received bytes
 */
int get_line (
    char* buff,
    int len)
{
    int c, i;

    i = 0;
    for (;;) {
        c = xfunc_in();            /* Get a char from the incoming stream */
        if (c=='\e'){
            return -1;            /* End of monitor */
        }
        if (!c){
            continue;            /* End of stream? */
        }
        if (c == '\r') {
            break;                /* End of line? */
        }
        if (c == '\b' && i) {    /* Back space? */
            i--;
            if (_LINE_ECHO) {
                xfunc_out(c);
            }
            xfunc_out(' ');
            xfunc_out(c);
            continue;
        }
        if (c >= ' ' && i < len - 1) {    /* Visible chars */
            buff[i++] = c;
            if (_LINE_ECHO){
                xfunc_out(c);
            }
        }
    }
    buff[i] = 0;    /* Terminate with zero */
    if (_LINE_ECHO){
        xfunc_out('\r');
        xfunc_out('\n');
    }
    return i;
}



/*
 * Monitor
 * @param : none
 * @return: none
 */
void monitor()
{
    char *ptr;
    unsigned long p1, p2, p3, ofs;
    int cnt, status, mode, repeate_mode;
    char backup[16];

    repeate_mode = 0;
    xfunc_out('>');

    for (;;) {
        ptr = Line;
        status = get_line(ptr, sizeof(Line));
        if(status > 0){
            repeate_mode = 0;
        }else if(status < 0){
            puts_direct("Exit\r\n");
            return;
        }
        if(repeate_mode && *ptr == 0){
            strcpy(ptr, backup);
        }

        switch (*ptr++) {
        case 'm' :
        {
            switch (*ptr++) {
            case 'd' :    /*  md <addr> Dump buffer */
                mode = *ptr++;
                if (!xatoi(&ptr, &p1)){
                    break;
                }
                if(monitor_address_check((unsigned int)p1)){
                    break;
                }
                for (ofs = p1, cnt = 16; cnt; cnt--, ofs+=16){
                    if(mode=='l'){
                        put_dump(ofs,  4, 4); /* mdl <addr> - Dump buffer with 4 bytes */
                        strcpy(backup, "mdl ");
                    }else if(mode=='w'){
                        put_dump(ofs,  8, 2); /* mdw <addr> - Dump buffer with 2 bytes  */
                        strcpy(backup, "mdw ");
                    }else{
                        put_dump(ofs, 16, 1); /* mdb <addr> - Dump buffer with byte  */
                        strcpy(backup, "mdb ");
                    }
                }
                repeate_mode = 1;
                sprintf(&backup[4], "0x%08x\r", (unsigned int)(ofs));
                break;

            case 's' :    /* ms <addr> [<data>] ... - Store R/W buffer */
                mode = *ptr++;
                if (!xatoi(&ptr, &p1)){
                    break;
                }
                if(monitor_address_check((unsigned int)p1)){
                    break;
                }
                if (xatoi(&ptr, &p2)) {
                    do {
                        *(char *)(p1++) = (char)p2;
                    } while (xatoi(&ptr, &p2));
                    break;
                }
                for (;;) {
                    if(mode=='l'){
                        sprintf(sprint_buffer, "%08X %08X-", (int)(p1), *(unsigned int *)p1);
                        puts_direct(sprint_buffer);
                    }else if(mode=='w'){
                        sprintf(sprint_buffer, "%08X %04X-", (int)(p1), *(unsigned short *)p1);
                        puts_direct(sprint_buffer);
                    }else{
                        sprintf(sprint_buffer, "%08X %02X-", (int)(p1), *(unsigned char *)p1);
                        puts_direct(sprint_buffer);
                    }
                    get_line(Line, sizeof(Line));
                    ptr = Line;
                    if (*ptr == '.'){
                        break;
                    }else if (*ptr == '-') {
                        if(mode=='l'){
                            p1-=4;
                        }else if(mode=='w'){
                            p1-=2;
                        }else{
                            p1--;
                        }
                        continue;
                    }else if (*ptr < ' ') {
                        if(mode=='l'){
                            p1+=4;
                        }else if(mode=='w'){
                            p1+=2;
                        }else{
                            p1++;
                        }
                        continue;
                    }else if (xatoi(&ptr, &p2)){
                        if(mode=='l'){
                            *(int *)p1 = (int)p2;
                            p1+=4;
                        }else if(mode=='w'){
                            *(short *)p1 = (short)p2;
                            p1+=2;
                        }else{
                            *(char *)p1++ = (char)p2;
                        }
                    }else{
                        puts_direct("???\r\n");
                    }
                }
                break;

            case 'f' :    /* mf <addr> <data> <size> - Fill working buffer */
                mode = *ptr++;
                if (!xatoi(&ptr, &p1)){
                    break;
                }
                if(monitor_address_check((unsigned int)p1)){
                    break;
                }
                if (!xatoi(&ptr, &p2)){
                    break;
                }
                if (!xatoi(&ptr, &p3)){
                    break;
                }
                if(mode=='l'){
                    for (cnt = (size_t)p3; cnt; cnt-=4, p1+=4){
                        *(int *)p1 = (int)p2;
                    }
                }else if(mode=='w'){
                    for (cnt = (size_t)p3; cnt; cnt-=2, p1+=2){
                        *(short *)p1 = (short)p2;
                    }
                }else{
                    for (cnt = (size_t)p3; cnt; cnt--, p1++){
                        *(char *)p1 = (char)p2;
                    }
                }
                break;
            }
            break;
        }    
        case 't' :
        {
            switch (*ptr++) {
            case 'h' :    /* th - free heap size */
                status_heap_size_view();
                break;

            case 'l' :    /* tl - task list */
                status_task_list_view();
                break;

            case 'r' :    /* tr - task run time state */
                status_task_runtime_view();
                break;
            }
            break;
        }
        case 'f' :
        {
            int i;
            switch (*ptr++) {
            case 'x' :{
                puts_direct("hard fault\r\n");
                *(int *)0x100000 = 0;
                __asm volatile ("nop");
                __asm volatile ("nop");
                __asm volatile ("nop");
                __asm volatile ("nop");
                __asm volatile ("nop");
                __asm volatile ("nop");
                break;
            }
            case 'c' :
                puts_direct("clear backup ram\r\n");
                for(i=10;i<20;i++){
                    rtc_backup_write(i, 0);
                }
                break;

            case 'd' :
                puts_direct("stored registers at last fault interrupt\r\n");
                sprintf(sprint_buffer, "r0  = 0x%08x\r\n", (unsigned int)rtc_backup_read(10));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "r1  = 0x%08x\r\n", (unsigned int)rtc_backup_read(11));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "r2  = 0x%08x\r\n", (unsigned int)rtc_backup_read(12));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "r3  = 0x%08x\r\n", (unsigned int)rtc_backup_read(13));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "r12 = 0x%08x\r\n", (unsigned int)rtc_backup_read(14));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "lr  = 0x%08x\r\n", (unsigned int)rtc_backup_read(15));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "pc  = 0x%08x\r\n", (unsigned int)rtc_backup_read(16));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "psr = 0x%08x\r\n", (unsigned int)rtc_backup_read(17));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "vector = %d\r\n", (unsigned int)rtc_backup_read(19));
                puts_direct(sprint_buffer);
                sprintf(sprint_buffer, "count  = %d\r\n", (unsigned int)rtc_backup_read(18));
                puts_direct(sprint_buffer);
                break;
            }
            break;
        }
        case 'r' :
        {
            rtc_time_display(1);
            break;
        }
        case 'x' :
        {
            switch (*ptr++) {
            case 'm' :
                puts_direct("direct modem console mode, wait until ppp link down\r\n");
                lwip_main_close(0);
                psld.direct_channel_modem = 1;
                return;
                break;

            case 'a' :
                puts_direct("direct arduino console mode\r\n");
                psld.direct_channel_arduino = 1;
                return;
                break;
            }
            break;
        }
        case '?' :
        {
            puts_direct("exit from monitor is to push ESC key\r\n\r\n");
            puts_direct("mdx <address>               : dump from <address> [x:b/w/l]\r\n");
            puts_direct("msx <address> <data>        : store <data> at <address> [x:b/w/l]\r\n");
            puts_direct("mfx <address> <data> <size> : fill <data> from <address> to <address+size> [x:b/w/l]\r\n");
            puts_direct("tl : task list[B:blocked, R:ready, D:deleted, S:suspend]\r\n");
            puts_direct("tr : run time states of task\r\n");
            puts_direct("th : free heap size\r\n");
            puts_direct("fc : clear backup memory\r\n");
            puts_direct("fd : display registers at last fault\r\n");
            puts_direct("xm : modem console\r\n");
            puts_direct("xa : arduino console\r\n");
            puts_direct("r  : display rtc calendar\r\n");
            puts_direct("\r\nhex value can be used. (example: 0x12, 0x1234, 0x12345678)\r\n");
            break;
        }
        default:
            break;
        }

        if(status>=0){
            xfunc_out('>');
        }
    }
}

