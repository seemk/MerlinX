#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "remote.h"
#include "console.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define LED PD5

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

uint8_t js_ref_avg_window[3] = { 130, 130, 130 };
uint8_t js_ref_avg_window_pos = 0;
uint8_t js_adc_x = 0;
uint8_t js_adc_y = 0;

uint8_t average(uint8_t* window, uint8_t size)
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
	joystick js;
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
			uint8_t avg = average(js_ref_avg_window, sizeof(js_ref_avg_window));
			js = map_js_reading(js_adc_x, js_adc_y, avg);
			console_write_fmt("JS_REF: %d\r\n", avg);
			console_write_fmt("JS_x: %d\r\n", js.x);
			console_write_fmt("JS_y: %d\r\n", js.y);
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
			js_adc_x = ADCH;
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
			js_adc_y = ADCH;
			// Switch to JS_1
			ADMUX = 0x65;
			break;
		default:
			break;
	}
	
	// Start the conversion again
	ADCSRA |= (1 << ADSC);
}

joystick map_js_reading(uint8_t adc_js_x, uint8_t adc_js_y, uint8_t adc_js_ref)
{
	// The reference has an average downward bias of 6 from the readings.
	static uint8_t const js_bias = 6;
	static int8_t const map_min = -100;
	static int8_t const map_max = 100;
	// The reference is ADC VCC / 2
	// Readings vary from reference +- 10% of VCC.
	// 25 here means ADC max (255) * 0.1
	int16_t adc_range_max = adc_js_ref + 25;
	int16_t adc_range_min = adc_js_ref - 25;

	int16_t ub_js_x = adc_js_x - js_bias;
	int16_t ub_js_y = adc_js_y - js_bias;
	
	int16_t offset = (map_max - map_min) / (adc_range_max - adc_range_min);
	
	joystick js;
	js.x = (ub_js_x - adc_range_min) * offset + map_min;
	js.y = (ub_js_y - adc_range_min) * offset + map_min;

	return js;
}