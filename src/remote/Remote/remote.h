/*
 * remote.h
 *
 * Created: 2.01.2014 20:06:41
 *  Author: Siim
 */ 


#ifndef REMOTE_H_
#define REMOTE_H_

typedef struct joystick {
	int8_t x;
	int8_t y;
} joystick;

typedef enum {
	CAM_CENTER,
	CAM_UP,
	CAM_DOWN,
	CAM_LEFT,
	CAM_RIGHT
} button_t;

// Maps the ADC readings to about -100 .. 100
joystick map_js_reading(uint8_t adc_js_x, uint8_t adc_js_y, uint8_t adc_js_ref);


#endif /* REMOTE_H_ */