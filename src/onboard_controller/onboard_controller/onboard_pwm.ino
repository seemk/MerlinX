int PWM_PIN = 8; // GREEN
int RIGHT_DIR = 52; // BLUE
int LEFT_DIR = 50; // WHITE
int IS_L = 51; // RED
int IS_R = 53; // ORANGE
int pwmVal = 100;

void setupPWM()
{
  pinMode(PWM_PIN, OUTPUT); 
  pinMode(RIGHT_DIR, OUTPUT);
  pinMode(LEFT_DIR, OUTPUT);
  
  pinMode(IS_L, INPUT);
  digitalWrite(IS_L, HIGH);
  pinMode(IS_R, INPUT);
  digitalWrite(IS_R, HIGH);
  
  digitalWrite(LEFT_DIR, LOW);
  digitalWrite(RIGHT_DIR, HIGH);
}
