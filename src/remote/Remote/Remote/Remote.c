#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "console.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define LED PD5

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

uint8_t js_ref_avg_window[3] = { 130, 130, 130 };
uint8_t js_ref_avg_window_pos = 0;
uint8_t js_x = 0;
uint8_t js_y = 0;

uint16_t average(uint8_t* window, uint8_t size)
{
	uint16_t avg = 0;
	for(uint8_t i = 0; i < size; ++i)
	{
		avg += window[i];
	}
	avg /= size;
	return avg;
}

void adc_init(void)
{
	// ADC frequency = F_CPU / 128
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
	// Enable ADC interrupts
	ADCSRA |= (1 << ADIE);
	// Enable free running
	ADCSRA |= (1 << ADATE);
	// ADC reference = AVCC, set by REFS0
	// ADCL is discarded, the result is in ADCH. Set by ADLAR
	ADMUX |= (1 << REFS0) | (1 << ADLAR);
	
	// Start conversion from an arbitrary ADC pin
	ADMUX |= 5;
	
	ADCSRA |= (1 << ADEN);
	sei();
	// Start with reading JS_REF
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

	adc_init();
	
	while(1)
	{
		console_attached = console_status(console_attached);
		if(console_attached)
		{
			output_high(PORTD, LED);
			console_recv_str(buf, sizeof(buf));
			console_send_str(PSTR("\r\n"));
			uint16_t avg = average(js_ref_avg_window, sizeof(js_ref_avg_window));
			console_write_fmt("JS_REF: %d\r\n", avg);
			console_write_fmt("JS_x: %d\r\n", js_x);
			console_write_fmt("JS_y: %d\r\n", js_y);
		}
		else
		{
			output_low(PORTD, LED);
		}
	}
}

ISR(ADC_vect)
{
	switch(ADMUX)
	{
		case 0x65:
			js_x = ADCH;
			// Switch to JS_REF
			ADMUX = 0x66;
			break;
		case 0x66:
			js_ref_avg_window[js_ref_avg_window_pos++] = ADCH;
			js_ref_avg_window_pos %= sizeof(js_ref_avg_window);
			// Switch to JS_2
			ADMUX = 0x67;
			break;
		case 0x67:
			js_y = ADCH;
			// Switch to JS_1
			ADMUX = 0x65;
			break;
		default:
			break;
	}
	
	// Start the conversion again
	ADCSRA |= (1 << ADSC);
	
}