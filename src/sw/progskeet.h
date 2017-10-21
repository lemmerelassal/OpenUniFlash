#ifndef _PROGSKEET_H
#define _PROGSKEET_H

#include <stdlib.h>
#include <stdint.h>

struct progskeet_handle;

int progskeet_init();

int progskeet_open(struct progskeet_handle** handle);

int progskeet_open_specific(struct progskeet_handle** handle, void* vdev);

int progskeet_close(struct progskeet_handle* handle);

int progskeet_reset(struct progskeet_handle* handle);

/* Sends until the TX buffer is empty */
int progskeet_sync(struct progskeet_handle* handle);

/* Cancels any running operation */
int progskeet_cancel(struct progskeet_handle* handle);



int progskeet_set_gpio_dir(struct progskeet_handle* handle, const uint16_t dir);

int progskeet_set_gpio(struct progskeet_handle* handle, const uint16_t gpio);

int progskeet_get_gpio(struct progskeet_handle* handle, uint16_t* gpio);

/* Waits for gpio & mask = addr before continuing */
int progskeet_wait_gpio(struct progskeet_handle* handle, const uint16_t mask, const uint16_t value);




int progskeet_set_addr(struct progskeet_handle* handle, const uint32_t addr, int auto_incr);




int progskeet_set_config(struct progskeet_handle* handle, const uint8_t delay, const int word);




/* Writes the buffer starting at the address that is currently set */
int progskeet_write(struct progskeet_handle* handle, const char* buf, const size_t len);

int progskeet_read(struct progskeet_handle* handle, char* buf, const size_t len);

int progskeet_write_addr(struct progskeet_handle* handle, uint32_t addr, uint16_t data);

int progskeet_read_addr(struct progskeet_handle* handle, uint32_t addr, uint16_t *data);




/* Does nothing by the specified ammount, 48 nops are 1us */
int progskeet_nop(struct progskeet_handle* handle, const uint8_t amount);

/* Approximately waits for the specified ammount of nanoseconds */
int progskeet_wait_ns(struct progskeet_handle* handle, const uint32_t ns);

/* Waits for the specified ammount of microseconds */
int progskeet_wait_us(struct progskeet_handle* handle, const uint32_t us);

/* Waits for the specified ammount of milliseconds */
int progskeet_wait_ms(struct progskeet_handle* handle, const uint32_t ms);

/* Waits for the specified ammount of seconds */
int progskeet_wait_s(struct progskeet_handle* handle, const uint32_t s);

#endif /* _PROGSKEET_H */
