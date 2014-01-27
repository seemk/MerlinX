#include "Arduino.h"


enum Button {
  NO_BUTTON,
  CAM_CENTER,
  CAM_UP,
  CAM_DOWN,
  CAM_LEFT,
  CAM_RIGHT,
  LIGHT,
  JOYSTICK,
  POT
};

enum Action {
        NO_ACTION,
	PRESSED,
	RELEASED,
	MOVED	
};

struct Joystick
{
  int x;
  int y;
};

struct Command
{
  Button button;
  Action action;
  Joystick joystick;
  byte hoverAdc;
};

enum Direction
{
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT
};

enum ShipMovementState
{
  STILL,
  FORWARD,
  BACKWARD,
  LEFT_TURN,
  RIGHT_TURN
};

struct Motor
{
  Motor() : direction(NONE), enabled(false) { } 
  Motor(int pin_pwm, int pin_dir_right, int pin_dir_left, int pin_sense_left, int pin_sense_right)
    : pwm_pin(pin_pwm)
    , right_dir_pin(pin_dir_right)
    , left_dir_pin(pin_dir_left)
    , sense_left_pin(pin_sense_left)
    , sense_right_pin(pin_sense_right)
    , pwm_value(100)
    , direction(NONE)
    , enabled(false)
    {  
      pinMode(pwm_pin, OUTPUT); 
      pinMode(right_dir_pin, OUTPUT);
      pinMode(left_dir_pin, OUTPUT);
  
      pinMode(sense_left_pin, INPUT);
      digitalWrite(sense_left_pin, HIGH);
      pinMode(sense_right_pin, INPUT);
      digitalWrite(sense_right_pin, HIGH);
  
      digitalWrite(left_dir_pin, LOW);
      digitalWrite(right_dir_pin, LOW);
    }

  Direction direction;
  bool enabled;
  int pwm_pin;
  int right_dir_pin;
  int left_dir_pin;
  int sense_left_pin;
  int sense_right_pin;
  
  int pwm_value;
  
  void setDirection(Direction dir)
  {
    switch(dir)
    {
      case NONE:
      {
        enabled = false;
        digitalWrite(left_dir_pin, LOW);
        digitalWrite(right_dir_pin, LOW);
      }
      break; 
      case LEFT:
      {
        enabled = true;
        digitalWrite(left_dir_pin, HIGH);
        digitalWrite(right_dir_pin, LOW);
      }
      break;
      case RIGHT:
      {
        enabled = true;
        digitalWrite(left_dir_pin, LOW);
        digitalWrite(right_dir_pin, HIGH);
      }
      break;
      default: break;
    }
  }
  void setPwm(int pwm) { pwm_value = pwm; }
  
  void update()
  {
    if(enabled)
    {
      analogWrite(pwm_pin, pwm_value); 
    }
  }
};
