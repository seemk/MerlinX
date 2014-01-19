#include "usart.h"
#include "defines.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "console.h"

void usart_init(uint32_t baud_rate)
{
	uint16_t prescale = (F_CPU / (baud_rate * 16UL)) - 1;
	// Enable USART transfer
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);
	// 8 bit character size
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
	UBRR1 = prescale;
	// Enable receive interrupts
	UCSR1B |= (1 << RXCIE1);
}

ISR(USART1_RX_vect)
{
	uint8_t incomingByte = UDR1;
	console_write_fmt("Received: %c\r\n", incomingByte);
}