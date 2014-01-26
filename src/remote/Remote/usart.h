#ifndef USART_H_
#define USART_H_

#include <stdint.h>

typedef void (*usart_callback_t)(uint8_t);

void usart_init(uint32_t baud_rate, usart_callback_t callback);
void usart_send(uint8_t byte);


#endif