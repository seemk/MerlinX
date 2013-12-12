#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "console.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define LED PD5

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

void setup_adc(void)
{
	// ADC frequency = F_CPU / 128
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
	// Enable ADC interrupts
	ADCSRA |= (1 << ADIE);
	// Enable free running
	ADCSRA |= (1 << ADATE);
	// ADC reference = AVCC
	ADMUX |= (1 << REFS0);
	// Set input from ADC7
	ADMUX |= (1 << MUX2) | (1 << MUX1) | (1 << MUX0);
	// The ADC result will be in ADCH, ADCL is discarded
	ADMUX |= (1 << ADLAR);
	
	ADCSRA |= (1 << ADEN);
	sei();
	ADCSRA |= (1 << ADSC);
}

int main(void)
{
	char buf[32];
	uint8_t console_attached = 0;
	CPU_PRESCALE(0);
	set_output(DDRD, LED);
	output_high(PORTD, LED);
	
	console_init();
	while(!console_configured()) { }
	_delay_ms(1000);
	output_low(PORTD, LED);
	//setup_adc();
	
	while(1)
	{
		console_attached = console_status(console_attached);
		if(console_attached)
		{
			output_high(PORTD, LED);
			console_send_str(PSTR("> "));
			console_recv_str(buf, sizeof(buf));
			console_send_str(PSTR("\r\n"));
		}
		else
		{
			output_low(PORTD, LED);
		}
	}
}

ISR(ADC_vect)
{
	if(ADCH < 107 || ADCH > 148)
	{
		output_high(PORTD, LED);
	}
	else
	{
		output_low(PORTD, LED);
	}
}