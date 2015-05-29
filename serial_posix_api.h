#ifndef SERIAL_POSIX_H
#define SERIAL_POSIX_H

#include <stdint.h>



serial * serial_create(void);
void serial_destroy(serial *obj);
int serial_open(serial *obj, const char *device);
int serial_mode_canonical(serial *obj);
int serial_mode_noncanonical(serial *obj);
int serial_set_speed(serial *obj, int speed);
int serial_set_timeout(serial *obj, unsigned int timeout);
int serial_set_minimum_read(serial *obj, unsigned int minimum_read);
int serial_set_cts_flow(serial *obj, int enabled);
size_t serial_get_nread(serial *obj);
size_t serial_read(serial *obj, uint8_t *buf, size_t size);
size_t serial_write(serial *obj, const uint8_t *buf, size_t size);
int serial_flush(serial *obj);
int serial_close(serial *obj);
int serial_get_fd(serial *obj);
int serial_get_errno(serial *obj);
int serial_posix_setup(const char *device, int baud, serial *serialobj);

#endif
