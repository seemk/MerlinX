#ifndef REMOTE_H_
#define REMOTE_H_

typedef struct {
	int8_t x;
	int8_t y;
} joystick_t;

typedef enum {
	NO_BUTTON,
	CAM_CENTER,
	CAM_UP,
	CAM_DOWN,
	CAM_LEFT,
	CAM_RIGHT,
	LIGHT,
	JS
} button_t;

typedef enum {
	NO_ACTION,
	PRESSED,
	RELEASED,
	MOVED
} action_t;

typedef struct {
	button_t button;
	action_t action;
	joystick_t joystick_reading;
} cmd_t;

// Maps the ADC readings to about -100 .. 100
joystick_t map_js_reading(uint8_t adc_js_x, uint8_t adc_js_y, uint8_t adc_js_ref);


#endif /* REMOTE_H_ */