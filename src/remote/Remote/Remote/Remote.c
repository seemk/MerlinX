/*
 * Remote.c
 *
 * Created: 3.12.2013 22:19:55
 *  Author: Siim
 */

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

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
	set_output(DDRD, LED);
	
	setup_adc();
	
    while(1)
    {
		
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