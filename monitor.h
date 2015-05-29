#ifndef MONITOR_H
#define MONITOR_H


void put_dump (unsigned long addr, int len, int);
int get_line (char* buff, int len);
int xatoi (char** str, unsigned long* res);
void monitor_configuration();
int monitor_address_check(unsigned int address);
void monitor();


#endif
