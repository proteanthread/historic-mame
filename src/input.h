#ifndef KEYBOARD_H
#define KEYBOARD_H


struct KeyboardInfo
{
	char *name;				/* OS dependant name; 0 terminates the list */
	UINT16 code;			/* OS dependant code (must be < KEYCODE_START) */
	UINT16 standardcode;	/* KEYCODE_xxx equivalent from list below, or KEYCODE_OTHER if n/a */
};

enum
{
	KEYCODE_START = 0xff00,	/* marker - not a real key */

	KEYCODE_1,KEYCODE_2,KEYCODE_3,KEYCODE_4,KEYCODE_5,
	KEYCODE_6,KEYCODE_7,KEYCODE_8,KEYCODE_9,KEYCODE_0,
	KEYCODE_A,KEYCODE_B,KEYCODE_C,KEYCODE_D,KEYCODE_E,KEYCODE_F,KEYCODE_G,
	KEYCODE_H,KEYCODE_I,KEYCODE_J,KEYCODE_K,KEYCODE_L,KEYCODE_M,KEYCODE_N,
	KEYCODE_O,KEYCODE_P,KEYCODE_Q,KEYCODE_R,KEYCODE_S,KEYCODE_T,KEYCODE_U,
	KEYCODE_V,KEYCODE_W,KEYCODE_X,KEYCODE_Y,KEYCODE_Z,
	KEYCODE_F1,KEYCODE_F2,KEYCODE_F3,KEYCODE_F4,KEYCODE_F5,KEYCODE_F6,
	KEYCODE_F7,KEYCODE_F8,KEYCODE_F9,KEYCODE_F10,KEYCODE_F11,KEYCODE_F12,
	KEYCODE_UP,KEYCODE_DOWN,KEYCODE_LEFT,KEYCODE_RIGHT,
	KEYCODE_ESC,KEYCODE_TAB,KEYCODE_ENTER,KEYCODE_SPACE,
	KEYCODE_LCONTROL,KEYCODE_RCONTROL,KEYCODE_LSHIFT,KEYCODE_RSHIFT,
	KEYCODE_LALT,KEYCODE_RALT,KEYCODE_BACKSPACE,KEYCODE_TILDE,
	KEYCODE_INSERT,KEYCODE_DEL,KEYCODE_HOME,KEYCODE_END,KEYCODE_PGUP,KEYCODE_PGDN,
	KEYCODE_1_PAD,KEYCODE_2_PAD,KEYCODE_3_PAD,KEYCODE_4_PAD,KEYCODE_5_PAD,
	KEYCODE_6_PAD,KEYCODE_7_PAD,KEYCODE_8_PAD,KEYCODE_9_PAD,KEYCODE_0_PAD,

	KEYCODE_OTHER,	/* anything else */

	KEYCODE_NONE	/* no key pressed */
};


const char *keyboard_name(int keycode);
int keyboard_pressed(int keycode);
int keyboard_pressed_memory(int keycode);
int keyboard_pressed_memory_repeat(int keycode,int speed);
int keyboard_read_async(void);
int keyboard_read_sync(void);


struct JoystickInfo
{
	char *name;				/* OS dependant name; 0 terminates the list */
	UINT16 code;			/* OS dependant code (must be < JOYCODE_START) */
	UINT16 standardcode;	/* JOYCODE_xxx equivalent from list below, or JOYCODE_OTHER if n/a */
};


enum
{
	JOYCODE_START = 0xff00,	/* marker - not a real joy */

	JOYCODE_1_LEFT,JOYCODE_1_RIGHT,JOYCODE_1_UP,JOYCODE_1_DOWN,
	JOYCODE_1_BUTTON1,JOYCODE_1_BUTTON2,JOYCODE_1_BUTTON3,
	JOYCODE_1_BUTTON4,JOYCODE_1_BUTTON5,JOYCODE_1_BUTTON6,
	JOYCODE_2_LEFT,JOYCODE_2_RIGHT,JOYCODE_2_UP,JOYCODE_2_DOWN,
	JOYCODE_2_BUTTON1,JOYCODE_2_BUTTON2,JOYCODE_2_BUTTON3,
	JOYCODE_2_BUTTON4,JOYCODE_2_BUTTON5,JOYCODE_2_BUTTON6,
	JOYCODE_3_LEFT,JOYCODE_3_RIGHT,JOYCODE_3_UP,JOYCODE_3_DOWN,
	JOYCODE_3_BUTTON1,JOYCODE_3_BUTTON2,JOYCODE_3_BUTTON3,
	JOYCODE_3_BUTTON4,JOYCODE_3_BUTTON5,JOYCODE_3_BUTTON6,
	JOYCODE_4_LEFT,JOYCODE_4_RIGHT,JOYCODE_4_UP,JOYCODE_4_DOWN,
	JOYCODE_4_BUTTON1,JOYCODE_4_BUTTON2,JOYCODE_4_BUTTON3,
	JOYCODE_4_BUTTON4,JOYCODE_4_BUTTON5,JOYCODE_4_BUTTON6,

	JOYCODE_OTHER,	/* anything else */

	JOYCODE_NONE	/* no key pressed */
};


const char *joystick_name(int joycode);
int joystick_pressed(int joycode);
int joystick_pressed_memory(int joycode);
int joystick_pressed_memory_repeat(int joycode,int speed);
int joystick_read_async(void);

/* the following read both key and joy */
int input_ui_pressed(int code);
int input_ui_pressed_repeat(int code,int speed);

#endif
