#include <Servo.h>
#include "types.h"

ShipMovementState movementState = STILL;

Motor leftMotor;
Motor rightMotor;

Servo servoH;
Servo servoV;

Joystick joystick = { 0, 0 };
Joystick average_js = { 0, 0 };
const int average_buf_size = 10;
int average_buf_idx = 0;
Joystick averages[average_buf_size];

const int servoOverflowMax = 75;
const int pwmLevelMin = 60;
const int pwmLevelMax = 250;

int hServoCenter = 68;
int hServoRight = 35;
int hServoLeft = 99;
int hServoCount = 0;
Direction hServoDirection = NONE;

int vServoCenter = 60;
int vServoDown = 79;
int vServoUp = 43;
int vServoCount = 0;
Direction vServoDirection = NONE;

const char cmd_begin = 0x7e;
const char cmd_end = 0x7f;

void moveVerticalServo(Direction dir)
{
  int current_position = servoV.read();
   vServoCount++;
  if(dir == UP)
  {
    
   int next_pos = current_position - (vServoCount / servoOverflowMax);
   if(next_pos >= vServoUp)
   {
     servoV.write(next_pos);
   }
  }
  else if(dir == DOWN)
  {
    int next_pos = current_position + (vServoCount / servoOverflowMax);
    if(next_pos <= vServoDown)
    {
      servoV.write(next_pos); 
    }
  }
  vServoCount %= servoOverflowMax;
}

void moveHorizontalServo(Direction dir)
{
   int current_position = servoH.read();
   hServoCount++;
   if(dir == RIGHT)
   {
     int next_pos = current_position - (hServoCount / servoOverflowMax);
     if(next_pos >= hServoRight)
     {
       servoH.write(next_pos);
     }
  }
  else if(dir == LEFT)
  {
    int next_pos = current_position + (hServoCount / servoOverflowMax);
    if(next_pos <= hServoLeft)
    {
      servoH.write(next_pos); 
    }
  }
  hServoCount %= servoOverflowMax;
}

void setupDCMotors()
{ 
  // PWM, right dir, left dir, left sense, right sense
  leftMotor = Motor(8, 9, 51, 53, 10);
  leftMotor.setPwm(100);
  
  rightMotor = Motor(11, 12, 50, 52, 13);
  rightMotor.setPwm(100);
}

void setup()
{
  memset(averages, 0, sizeof(averages));
  // Serial = terminal
  Serial.begin(9600);
  // Serial1 = from RS422 module
  Serial1.begin(9600);
  
  servoH.attach(2);
  servoV.attach(3);
  servoH.write(hServoCenter);
  servoV.write(vServoCenter);
  
  setupDCMotors();
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

void handleCmd(const Command& cmd)
{
  Button btn = cmd.button;
  Action action = cmd.action;
  if(btn == CAM_CENTER && action == PRESSED)
  {
   servoH.write(hServoCenter);
   servoV.write(vServoCenter); 
  }
  
  switch(action)
  {
   case PRESSED:
   {
     if(btn == CAM_DOWN) vServoDirection = DOWN;
     if(btn == CAM_UP) vServoDirection = UP;
     if(btn == CAM_LEFT) hServoDirection = LEFT;
     if(btn == CAM_RIGHT) hServoDirection = RIGHT;
   }
   break;
   case RELEASED:
   {
     if(btn == CAM_DOWN || btn == CAM_UP) vServoDirection = NONE;
     if(btn == CAM_LEFT || btn == CAM_RIGHT) hServoDirection = NONE;
   }
   break;
   case MOVED:
   {
     if(btn == JOYSTICK)
     {
       const int maxDeviation = 50;
       Joystick reading = cmd.joystick;
       // Discard the reading if the value jumps too rapidly
       if(abs(reading.x - average_js.x) < maxDeviation && abs(reading.y - average_js.y))
       {
         averages[average_buf_idx++] = reading;
         average_buf_idx %= average_buf_size;
         Joystick summed_reading = { 0, 0 };
         for(int i = 0; i < average_buf_size; i++)
         {
           summed_reading.x += averages[i].x;
           summed_reading.y += averages[i].y; 
         }
         
         average_js.x = summed_reading.x / average_buf_size;
         average_js.y = summed_reading.y / average_buf_size;
       }
       
     }
   }
   break;
   default: break;
  }
}

int isCommandValid(const Command& cmd)
{
 Action action = cmd.action;
 Button btn = cmd.button;
 return btn <= 7 && action <= 3; 
}

void printJoystick(Joystick js)
{
    Serial.print(" JS: (");
    Serial.print(js.x, DEC);
    Serial.print(", ");
    Serial.print(js.y, DEC);
    Serial.print(")");
}

void printCmd(const Command& cmd)
{
  Serial.print("Command { Button: ");
  Serial.print(cmd.button, DEC);
  Serial.print(" Action: ");
  Serial.print(cmd.action, DEC);
  if(cmd.button == JOYSTICK)
  {
    printJoystick(cmd.joystick);
  }

  Serial.println(" }");
}

void moveServos()
{
  moveVerticalServo(vServoDirection);
  moveHorizontalServo(hServoDirection);
}

void loop()
{
  Command cmd;
  int bufSize = 8;
  char buf[bufSize];
  memset(buf, 0, bufSize);
  int idx = 0;
  moveServos();
  if(Serial1.available())
  {
    Serial1.readBytesUntil(cmd_end, buf, bufSize);
    if(parseCmd(buf, cmd))
    {
      if(isCommandValid(cmd))
      {
        //printCmd(cmd);
        handleCmd(cmd);
        //printJoystick(average_js);
        //Serial.println("");
        
        if(average_js.y > 25)
        {
            movementState = FORWARD;
            
        }
        else
        {
           movementState = STILL;
           
        }
        
        switch(movementState)
        {
         case STILL:
         {
           leftMotor.setDirection(NONE);
           rightMotor.setDirection(NONE);
         }
         break;
         case FORWARD:
         {
           rightMotor.setDirection(LEFT);
           leftMotor.setDirection(LEFT);
         }
         break;
        }
        
      }

    }
    else
    {
       Serial.println("Parse error"); 
    }
  }
 
  leftMotor.update();
  rightMotor.update();
}
