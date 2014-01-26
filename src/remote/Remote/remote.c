#include "defines.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "remote.h"
#include "console.h"
#include "usart.h"

uint8_t button_states[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
joystick_t joystick_reading = { 0, 0 };

void send_cmd(cmd_t cmd)
{
	usart_send(cmd_begin);
	usart_send(cmd.button);
	usart_send(cmd.action);
	usart_send(cmd.joystick_reading.x);
	usart_send(cmd.joystick_reading.y);
	usart_send(cmd_end);
}

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

void usart_byte_received(uint8_t incoming)
{
	console_write_fmt("Received: %c", incoming);
	usart_send(incoming);
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

void buttons_init()
{

	set_input(DDRC, BTN_CAM_DOWN);
	output_high(PORTC, BTN_CAM_DOWN);
	
	set_input(DDRC, BTN_CAM_UP);
	output_high(PORTC, BTN_CAM_UP);
	
	set_input(DDRB, BTN_CAM_CENTER);
	output_high(PORTB, BTN_CAM_CENTER);
	
	set_input(DDRB, BTN_CAM_LEFT);
	output_high(PORTB, BTN_CAM_LEFT);
	
	set_input(DDRB, BTN_CAM_RIGHT);
	output_high(PORTB, BTN_CAM_RIGHT);
	
	set_input(DDRD, BTN_LIGHT);
	output_high(PORTD, BTN_LIGHT);
	
	set_input(DDRD, BTN_UU1);
	output_high(PORTD, BTN_UU1);
	
	set_input(DDRE, BTN_UU2);
	output_high(PORTE, BTN_UU2);
	
	set_input(DDRB, BTN_UU3);
	output_high(PORTB, BTN_UU3);
	
	set_input(DDRB, BTN_UU4);
	output_high(PORTB, BTN_UU4);
}

void update_buttons(uint8_t* btn_array)
{
	btn_array[CAM_CENTER] = BTN_PRESSED(PINB, BTN_CAM_CENTER);
	btn_array[CAM_UP]	  = BTN_PRESSED(PINC, BTN_CAM_UP);
	btn_array[CAM_DOWN]	  = BTN_PRESSED(PINC, BTN_CAM_DOWN);
	btn_array[CAM_LEFT]   = BTN_PRESSED(PINB, BTN_CAM_LEFT);
	btn_array[CAM_RIGHT]  = BTN_PRESSED(PINB, BTN_CAM_RIGHT);
	btn_array[LIGHT]	  = BTN_PRESSED(PIND, BTN_LIGHT);
}

void print_btn_states(uint8_t* btn_array)
{
	console_write_fmt("Up: %d\r\n", btn_array[CAM_UP]);
	console_write_fmt("Down: %d\r\n", btn_array[CAM_DOWN]);
	console_write_fmt("Center: %d\r\n", btn_array[CAM_CENTER]);
	console_write_fmt("Left: %d\r\n", btn_array[CAM_LEFT]);
	console_write_fmt("Right: %d\r\n", btn_array[CAM_RIGHT]);
	console_write_fmt("Light: %d\r\n", btn_array[LIGHT]);
}

// Returns NO_BUTTON if the states are equal
// Otherwise returns the button whose state was changed
button_t compare_states(uint8_t* a, uint8_t* b)
{
	if(a[CAM_CENTER] != b[CAM_CENTER]) return CAM_CENTER;
	if(a[CAM_UP] != b[CAM_UP])		   return CAM_UP;
	if(a[CAM_DOWN] != b[CAM_DOWN])	   return CAM_DOWN;
	if(a[CAM_LEFT] != b[CAM_LEFT])	   return CAM_LEFT;
	if(a[CAM_RIGHT] != b[CAM_RIGHT])   return CAM_RIGHT;
	if(a[LIGHT] != b[LIGHT])		   return LIGHT;
	
	return NO_BUTTON;
}

uint8_t joystick_needs_command(joystick_t* old_js, joystick_t* new_js)
{
	const int8_t min_deviation = 6;
	int x_deviation = abs(old_js->x - new_js->x);
	int y_deviation = abs(old_js->y - new_js->y);
	return ( x_deviation > min_deviation || y_deviation > min_deviation);
}

int main(void)
{
	char buf[32];
	uint8_t console_attached = 0;
	CPU_PRESCALE(0);
	set_output(DDRD, LED);
	output_high(PORTD, LED);
	
	buttons_init();
	
	usart_init(9600, &usart_byte_received);
	console_init();
	
	while(!console_configured()) { }
	_delay_ms(1000);
	output_low(PORTD, LED);

	adc_init();
	
	while(1)
	{
		
		uint8_t avg = average(js_ref_avg_window, sizeof(js_ref_avg_window));
		joystick_t new_js_reading = map_js_reading(js_adc_x, js_adc_y, avg);
		
		// Handle joystick readings, only send the JS command if the new reading
		// deviates from the old reading by a certain amount
		//if(joystick_needs_command(&joystick_reading, &new_js_reading))
		//{
			//cmd_t cmd = { JS, MOVED, new_js_reading };
			//send_cmd(cmd);
		//}
				
		joystick_reading = new_js_reading;
		
		uint8_t new_btn_states[8];
		update_buttons(new_btn_states);
		button_t changed_btn = compare_states(button_states, new_btn_states);
		if(changed_btn != NO_BUTTON)
		{
			action_t action = new_btn_states[changed_btn] ? PRESSED : RELEASED;
			cmd_t cmd = { changed_btn, action, new_js_reading };
			console_write_fmt("Sending btn command.");
			send_cmd(cmd);
		}
		memcpy(button_states, new_btn_states, sizeof(button_states));
		
		console_attached = console_status(console_attached);
		if(console_attached)
		{
			output_high(PORTD, LED);
			//console_recv_str(buf, sizeof(buf));
			//console_send_str(PSTR("\r\n"));
			//print_btn_states(new_btn_states);
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

joystick_t map_js_reading(uint8_t adc_js_x, uint8_t adc_js_y, uint8_t adc_js_ref)
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
	
	joystick_t js;
	js.x = (ub_js_x - adc_range_min) * offset + map_min;
	js.y = (ub_js_y - adc_range_min) * offset + map_min;

	return js;
}