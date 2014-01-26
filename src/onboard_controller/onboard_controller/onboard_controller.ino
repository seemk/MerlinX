#include <Servo.h>
#include "types.h"

Servo servoH;
Servo servoV;

int hServoCenter = 68;
int hServoRight = 35;
int hServoLeft = 99;
int vServoCenter = 60;
int vServoDown = 79;
int vServoUp = 43;

const char cmd_begin = 0x7e;
const char cmd_end = 0x7f;


void setup()
{
  // Serial = terminal
  Serial.begin(9600);
  // Serial1 = from RS422 module
  Serial1.begin(9600);
  
  servoH.attach(2);
  servoV.attach(3);
  servoH.write(hServoCenter);
  servoV.write(vServoCenter);
}

// Returns 0 for error, 1 for success
int parseCmd(char* buf, Command& cmd)
{
  if(buf[0] != cmd_begin) return 0;
  
  cmd.button = (Button)buf[1];
  cmd.action = (Action)buf[2];
  cmd.joystick.x = (int)buf[3];
  cmd.joystick.y = (int)buf[4];
  
  return 1;  
}

void printCmd(const Command& cmd)
{
  Serial.print("Command { Button: ");
  Serial.print(cmd.button, DEC);
  Serial.print(" Action: ");
  Serial.print(cmd.action, DEC);
  if(cmd.button == JOYSTICK)
  {
    Serial.print(" JS: (");
    Serial.print(cmd.joystick.x, DEC);
    Serial.print(", ");
    Serial.print(cmd.joystick.y, DEC);
    Serial.print(")");
  }

  Serial.println(" }");
}

void loop()
{
  Command cmd;
  int bufSize = 8;
  char buf[bufSize];
  memset(buf, 0, bufSize);
  int idx = 0;
  if(Serial1.available())
  {
    Serial1.readBytesUntil(cmd_end, buf, bufSize);
    Serial.println("Read bytes finished.");
    if(parseCmd(buf, cmd))
    {
      printCmd(cmd); 
    }
    else
    {
       Serial.println("Parse error"); 
    }
  }
  //analogWrite(PWM_PIN, pwmVal); 
}
