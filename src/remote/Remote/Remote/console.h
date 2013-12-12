#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdint.h>

void console_init();
uint8_t console_configured();
uint8_t console_status(uint8_t prev_status);

void console_send_str(const char *s);
uint8_t console_recv_str(char *buf, uint8_t size);

#endif /* CONSOLE_H_ */