#include <DueTimer.h>
#include "types.h"

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

void setup() {
  /*
  Serial = PC, debug output
  Serial1 = DATA between Arduinos
  Serial2 = Debug line between Arduinos
  */
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  
  
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

void loop() {
  
  jsAdcX = analogRead(jsXPin);
  jsAdcY = analogRead(jsYPin);
  int refRead = analogRead(jsRefPin);
  jsRefAvgWindow[jsRefWindowPosition++] = refRead;
  jsRefWindowPosition %= windowSize;

  // Only JS at the moment.
  Command cmd;
  cmd.button = JOYSTICK;
  cmd.action = MOVED;
  uint16_t referenceAverage = average(jsRefAvgWindow, windowSize);
  Joystick js = mapJoystickReadings(jsAdcX, jsAdcY, referenceAverage);
  cmd.joystick = js;
  cmd.hoverAdc = 80;
  sendCommand(cmd);
  
  // Handle debug messages
  if(Serial2.available()) {
    char inByte = Serial2.read();
    // Write to PC
    Serial.write(inByte); 
  }
}
