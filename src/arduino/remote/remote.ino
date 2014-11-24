#include <DueTimer.h>
#include "types.h"

struct ButtonState {
  ButtonState()
    : pin(0), needsUpdate(0), pressed(0) { }
    
  ButtonState(int pin)
    : pin(pin), needsUpdate(0), pressed(0) {
      pinMode(pin, INPUT_PULLUP);
  }

  void readPin() {
    
    // Don't read uninitialized button state pins
    if(pin == 0) return;
    
    uint8_t newPress = !digitalRead(pin);
    
    // If the previous state differs from the new state, set the update flag.
    // If the update flag is set, a command gets sent and the update flag is zeroed.
    if (newPress != pressed) {
      needsUpdate = 1;
      pressed = newPress;
    }
  }
  
  int pin;
  uint8_t needsUpdate;
  uint8_t pressed;
};

// Analog pins for the joystick
// Green wire, JS reference (Input voltage / 2) = Analog 0
// Blue wire, x direction = Analog 1
// Yellow wire, y direction = Analog 2
const int jsRefPin = 0;
const int jsXPin = 1;
const int jsYPin = 2;

// Joystick readings
const int windowSize = 5;
uint16_t jsRefAvgWindow[windowSize];
// Current position in the average window
int jsRefWindowPosition = 0;
uint16_t jsAdcX = 0;
uint16_t jsAdcY = 0;
uint16_t hoverAdc = 0;

// Digital buttons handled here.
// buttons[BUTTON_NAME] = 0/1
// E.g. buttons[CAM_CENTER] = 1 if it's pressed
ButtonState buttons[BUTTON_COUNT];

uint16_t average(const uint16_t* elems, int length) {
  // Avoid overflows, 16 bits is enough
  uint32_t avg = 0;
  for(int i = 0; i < length; ++i)
  {
    avg += elems[i];
  }
  avg /= length;
  return avg; 
}

// Maps the ADC readings to -100 .. 100
Joystick mapJoystickReadings(uint16_t adc_js_x, uint16_t adc_js_y, uint16_t adc_js_ref)
{
  const float map_min = -100.f;
  const float map_max = 100.f;
  // The reference (green) is ADC VCC / 2
  // Readings (yellow, blue) vary from reference +- 10% of VCC.
  // Vref = 5V, analogRead() max is 1023, same as VCC
  const float adc_ref = 1023.f;
  const float adc_offset = 0.1f * adc_ref;
  const float adc_range_max = adc_js_ref + adc_offset;
  const float adc_range_min = adc_js_ref - adc_offset;
  const float map_interval = map_max - map_min;
  const float adc_interval = adc_range_max - adc_range_min;
  const float offset = map_interval / adc_interval;
  Joystick js;
  js.x = (adc_js_x - adc_range_min) * offset + map_min;
  js.y = (adc_js_y - adc_range_min) * offset + map_min;

  return js;
}

void sendCommand(const Command& cmd) {
  Serial1.write(cmd_begin);
  Serial1.write(static_cast<byte>(cmd.button));
  Serial1.write(static_cast<byte>(cmd.action));
  Serial1.write(static_cast<byte>(cmd.joystick.x));
  Serial1.write(static_cast<byte>(cmd.joystick.y));
  Serial1.write(static_cast<byte>(cmd.hoverAdc));
  Serial1.write(cmd_end);
}

void initButtons() {
  // Don't forget to reroute pins!
  // The pins here are just an example.
  buttons[CAM_CENTER] = ButtonState(22);
  buttons[CAM_UP] = ButtonState(24);
  buttons[CAM_DOWN] = ButtonState(26);
  buttons[CAM_LEFT] = ButtonState(28);
  buttons[CAM_RIGHT] = ButtonState(30);
}

void updateButtons() {
  for(int i = 0; i < BUTTON_COUNT; i++) {
    ButtonState& state = buttons[i];
    state.readPin();
    if(state.needsUpdate) {
      
      Command cmd;
      cmd.button = static_cast<Button>(i); // Looping over enum values, i = NO_BUTTON .. HOVER_INCREASE
      
      if(state.pressed) {
        cmd.action = PRESSED;
      }
      else {
        cmd.action = RELEASED; 
      }
      
      cmd.joystick.x = 0;
      cmd.joystick.y = 0;
      cmd.hoverAdc = 0;
      
      state.needsUpdate = false; 
    }
  }
}

void setup() {
  /*
  Serial = PC, debug output
  Serial1 = DATA between Arduinos
  Serial2 = Debug line between Arduinos
  */
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  
  initButtons();
  
}

void loop() {
  
  jsAdcX = analogRead(jsXPin);
  jsAdcY = analogRead(jsYPin);
  int refRead = analogRead(jsRefPin);
  jsRefAvgWindow[jsRefWindowPosition++] = refRead;
  jsRefWindowPosition %= windowSize;
 

  Command cmd;
  cmd.button = JOYSTICK;
  cmd.action = MOVED;
  uint16_t referenceAverage = average(jsRefAvgWindow, windowSize);
  Joystick js = mapJoystickReadings(jsAdcX, jsAdcY, referenceAverage);
  cmd.joystick = js;
  cmd.hoverAdc = 80;
  sendCommand(cmd);
  
  // Read new button states and send commands if necessary
  updateButtons();
  
  // Handle debug messages
  while(Serial2.available()) {
    char inByte = Serial2.read();
    // Write to PC
    Serial.write(inByte); 
  }
  
  delay(50); // TODO: Move to timers
}
