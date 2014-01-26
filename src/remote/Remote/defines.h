#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define F_CPU 16000000UL
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

// IO pins
#define LED PD5
#define BTN_CAM_UP PC7
#define BTN_CAM_DOWN PC6
#define BTN_CAM_CENTER PB6
#define BTN_CAM_LEFT PB5
#define BTN_CAM_RIGHT PB4
#define BTN_LIGHT PD7
// Unused pins, currently placeholders for extra buttons. Should still be set as inputs
#define BTN_UU1 PD6
#define BTN_UU2 PE6
#define BTN_UU3 PB0
#define BTN_UU4 PB7

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

#define BTN_PRESSED(pin, btn) (!(pin & (1 << btn)))
#define cmd_begin 0x7e
#define cmd_end   0x7f

#endif