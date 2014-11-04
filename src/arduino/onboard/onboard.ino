#include <Servo.h>
#include "types.h"

#define DEBUG_LOGS 0

ShipMovementState movementState = STILL;

Motor leftMotor;
Motor rightMotor;
Motor hoverMotor;

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
const int movementThreshold = 20;
const int minHoverPwm = 50;
const int maxHoverPwm = 230;
const int hoverPwmStep = 20;

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

byte hoverADC = 0;

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
  
  rightMotor = Motor(11, 12, 50, 52, 13);
  
  hoverMotor = Motor(7, 6, 48, 46, 5);
  hoverMotor.setPwm(150);
}

void setup()
{
  memset(averages, 0, sizeof(averages));
  // Serial = terminal
  Serial2.begin(9600);
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
  cmd.hoverAdc = (byte)buf[5];
  
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
  
  if(btn == POT)
  {
    hoverADC = cmd.hoverAdc;
    Serial2.print("Received hover ADC. ");
    Serial2.println(hoverADC, DEC);
    return;
  }
  switch(action)
  {
   case PRESSED:
   {
     if(btn == CAM_DOWN) vServoDirection = DOWN;
     if(btn == CAM_UP) vServoDirection = UP;
     if(btn == CAM_LEFT) hServoDirection = LEFT;
     if(btn == CAM_RIGHT) hServoDirection = RIGHT;
     if(btn == HOVER_DECREASE)
     {
       if(hoverMotor.enabled)
       {
         int nextPwm = hoverMotor.getPwm() - hoverPwmStep;
         if(nextPwm >= minHoverPwm)
         {
            hoverMotor.setPwm(nextPwm);
         }
         else
         {
           hoverMotor.setDirection(NONE);
         }
       }

       
     }
     if(btn == HOVER_INCREASE)
     {
       if(hoverMotor.enabled)
       {
         int nextPwm = hoverMotor.getPwm() + hoverPwmStep;
         if(nextPwm <= maxHoverPwm) hoverMotor.setPwm(nextPwm);
       }
       else
       {
          hoverMotor.setDirection(LEFT);
       }
       
     }
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
 return btn <= 10 && action <= 3; 
}

void printJoystick(Joystick js)
{
    Serial2.print(" JS: (");
    Serial2.print(js.x, DEC);
    Serial2.print(", ");
    Serial2.print(js.y, DEC);
    Serial2.print(")");
}

void printCmd(const Command& cmd)
{
  Serial2.print("Command { Button: ");
  Serial2.print(cmd.button, DEC);
  Serial2.print(" Action: ");
  Serial2.print(cmd.action, DEC);
  if(cmd.button == JOYSTICK)
  {
    printJoystick(cmd.joystick);
  }

  Serial2.println(" }");
}

// Maps Joystick reading [-100, 100] to [60, 250]
int joystickToPwm(int joystickValue)
{
  const float pwmLevelMin = 30.f;
  const float pwmLevelMax = 240.f;
  int absoluteJsValue = abs(joystickValue);
  if(absoluteJsValue <= movementThreshold)
  {
    return pwmLevelMin; 
  }
  else if(absoluteJsValue >= 100)
  {
    return pwmLevelMax; 
  }
  else
  {
    float converted = (((float)absoluteJsValue - movementThreshold) / 80.f) * (pwmLevelMax - pwmLevelMin) + pwmLevelMin;
    return (int)converted;
  }
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
        #if DEBUG_LOGS
        printCmd(cmd);
        #endif
        handleCmd(cmd);

        if(average_js.y > movementThreshold)
        {
            movementState = FORWARD;
            
        }
        else if(average_js.y < -movementThreshold)
        {
           movementState = BACKWARD;
        }
        else if(average_js.x < -movementThreshold)
        {
          movementState = LEFT_TURN;
        }
        else if(average_js.x > movementThreshold)
        {
          movementState = RIGHT_TURN;
        }
        else
        {
           movementState = STILL;
        }
        
        int pwm_horizontal = joystickToPwm(average_js.x);
        int pwm_vert = joystickToPwm(average_js.y);
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
           rightMotor.setDirection(RIGHT);
           leftMotor.setDirection(RIGHT);
           rightMotor.setPwm(pwm_vert);
           leftMotor.setPwm(pwm_vert);
         }
         break;
         case BACKWARD:
         {
           rightMotor.setDirection(LEFT);
           leftMotor.setDirection(LEFT); 
           rightMotor.setPwm(pwm_vert);
           leftMotor.setPwm(pwm_vert);
         }
         break;
         case LEFT_TURN:
         {
           rightMotor.setDirection(RIGHT);
           rightMotor.setPwm(pwm_horizontal);
           leftMotor.setDirection(NONE); 
         }
         break;
         case RIGHT_TURN:
         {
           rightMotor.setDirection(NONE);
           leftMotor.setDirection(RIGHT);
           leftMotor.setPwm(pwm_horizontal);
         }
         break;
        }
        
      }

    }
    else
    {
       Serial2.println("Parse error"); 
    }
  }
 
  leftMotor.update();
  rightMotor.update();
  hoverMotor.update();
}
