
enum Button {
  NO_BUTTON,
  CAM_CENTER,
  CAM_UP,
  CAM_DOWN,
  CAM_LEFT,
  CAM_RIGHT,
  LIGHT,
  JS
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
};


