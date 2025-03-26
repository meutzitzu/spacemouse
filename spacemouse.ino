#include <string.h>
#include <BleCombo.h>

/*
	the spacemouse is comprised of 3 joysticks arranged in a triangular orientation
	we name these A, B, C (the foreward one is A and the rest are incremented in the trigonometric direction)
	the U basis of each joystick is positive towards the trigonometric direction
	the V basis of each joystick is positive upwards
	     _
	    ╱A╲
	   ╱   ╲
	  ╱     ╲
	 ╱       ╲
	\B       C/
	 🭶🭶🭶🭶🭶🭶🭶🭶🭶 
	   <60> mm
*/

#define sin60 1000/866
#define sin30 2/1

#define ANALOG_MAX 4096 // measuring range of ADCs
#define THRESHOLD 1024 // "deadzone" size. any input below this will be ignored
#define FILTERING 1  // number of rolling average slots. higher values reduce noise at the cost of input lag
#define DELAY 8 // use this to control the main loop speed
#define SPEED 1/16

BleCombo bleCombo("Space Mouse", "un jmeker", 42);

bool button;
bool prev_button;
bool button_event;
bool enabled;

enum Pinout:byte
{ // current pin configuration
	STICK0_U = 12,
	STICK0_V = 13,
	STICK1_U = 27,
	STICK1_V = 14,
	STICK2_U = 25,
	STICK2_V = 26,
	BUTTON = 18,
};


struct roll //raw channel input rotating buffer
{
	int index; //used for rotating buffer indexing
	int offset;//used for calibrating around 0
	int val[FILTERING];//actual buffer
};

int normalize(struct roll roller)
{ // centers the potentiometer values around 0
	return roll_avg(roller)-roller.offset;
}

void roll_zero(struct roll *roller)
{
	roller->offset=roll_avg(*roller);
}


void roll_update(struct roll *roller, int newval)
{
	roller->val[roller->index]=newval;
	roller->index=(roller->index + 1)%FILTERING;
}


int roll_avg( struct roll roller)
{
	int avg = 0;
	// pretty sure this can be done in a much more clever way, without using a for
	for (int i=0; i<FILTERING; ++i)
	{
		avg += roller.val[i];
	}
	avg /= FILTERING;
	return avg;
}

struct
{
	struct roll Au;
	struct roll Av;
	struct roll Bu;
	struct roll Bv;
	struct roll Cu;
	struct roll Cv;
} raw_rolling;

void zero_all()
{
	roll_zero(&raw_rolling.Au);
	roll_zero(&raw_rolling.Av);
	roll_zero(&raw_rolling.Bu);
	roll_zero(&raw_rolling.Bv);
	roll_zero(&raw_rolling.Cu);
	roll_zero(&raw_rolling.Cv);
}

struct uv
{ // we will work with 2d vectors a lot
	int u;
	int v;
};

struct
{ // normalized local coordinates of each sticks
	struct uv A;
	struct uv B;
	struct uv C;
}input;

struct
{ // movement increments on each motion
	int zoom;
	int roll;
	struct uv pan;
	struct uv orbit;
}motion;

struct active_channel
{ // whether the current channel has active input (more than threshold)
	bool zoom;
	bool roll;
	bool pan;
	bool orbit;
};

 // we need this for edge detection
struct active_channel curr;
struct active_channel prev;



void updateInput()
{ // read input values from analog ports
	button = !digitalRead(Pinout::BUTTON);
	button_event = (button && !prev_button ? true : false);
	enabled = (button_event ? !enabled : enabled);
	Serial.print((enabled ? "ON" : "OFF"));
	Serial.print("\t");
	Serial.print((button_event ? "ON" : "OFF"));
	Serial.print("\t");
	Serial.println((button ? "ON" : "OFF"));
	prev_button = button;
	
	roll_update(&raw_rolling.Au, analogRead(Pinout::STICK0_U));
	roll_update(&raw_rolling.Av, analogRead(Pinout::STICK0_V));
	roll_update(&raw_rolling.Bu, analogRead(Pinout::STICK1_U));
	roll_update(&raw_rolling.Bv, analogRead(Pinout::STICK1_V));
	roll_update(&raw_rolling.Cu, analogRead(Pinout::STICK2_U));
	roll_update(&raw_rolling.Cv, analogRead(Pinout::STICK2_V));
	
	input.A.u = normalize(raw_rolling.Au);
	input.A.v = normalize(raw_rolling.Av);
	input.B.u = normalize(raw_rolling.Bu);
	input.B.v = normalize(raw_rolling.Bv);
	input.C.u = normalize(raw_rolling.Cu);
	input.C.v = normalize(raw_rolling.Cv);
}

void plotInputs()
{
	Serial.print(input.A.u);
	Serial.print("\t");
	Serial.print(input.A.v);
	Serial.print("\t");
	Serial.print(input.B.u);
	Serial.print("\t");
	Serial.print(input.B.v);
	Serial.print("\t");
	Serial.print(input.C.u);
	Serial.print("\t");
	Serial.print(input.C.v);
	Serial.print("\r\n");
}
 
void printInputs()
{
	Serial.print("INPUT:\t ");
	Serial.print("\tAu=\t ");
	Serial.print(input.A.u);
	Serial.print("\tAv=\t ");
	Serial.print(input.A.v);
	Serial.print("\tBu=\t ");
	Serial.print(input.B.u);
	Serial.print("\tBv=\t ");
	Serial.print(input.B.v);
	Serial.print("\tCu=\t ");
	Serial.print(input.C.u);
	Serial.print("\tCv=\t ");
	Serial.print(input.C.v);
	Serial.print("\r\n");
}

void printMotions()
{
	Serial.print("INPUT:   ");
	Serial.print("\tZoom=\t ");
	Serial.print(motion.zoom);
	Serial.print("\tPanX=\t ");
	Serial.print(motion.pan.u);
	Serial.print("\tPanY=\t ");
	Serial.print(motion.pan.v);
	Serial.print("\tOrbitX=\t ");
	Serial.print(motion.orbit.u);
	Serial.print("\tOrbitY=\t ");
	Serial.print(motion.orbit.v);
	Serial.print("\r\n");
}

void plotMotions()
{
	Serial.print(motion.zoom);
	Serial.print("\t");
	Serial.print(motion.pan.u);
	Serial.print("\t");
	Serial.print(motion.pan.v);
	Serial.print("\t");
	Serial.print(motion.orbit.u);
	Serial.print("\t");
	Serial.print(motion.orbit.v);
	Serial.print("\t");
	Serial.print(motion.roll);
	Serial.print("\r\n");
}

/*
 * THIS IS WHERE THE MAGIC HAPPENS
 * everythig else is mostly just boilerplate for reading values
 * and simulating keyboard&mouse inputs
 * This is where the kinematic expressions calculate
 * the movement on all channels of the 3D transform
 * from each stick's local UV space
 */

void calcMotion()
{ // calculates the motions from the states of the joysticks
	motion.zoom = (input.A.v + input.B.v + input.C.v)/3;
	motion.roll = (input.A.u + input.B.u + input.C.u)/3;
	motion.pan = 
	{
		-input.A.u + (input.B.u + input.C.u)*sin30, 
		(-input.B.u + input.C.u)*sin60
	};
	motion.orbit=
	{
		-(input.B.v - input.C.v)*sin60, 
		-input.A.v + (input.B.v + input.C.v)*sin30
	};
	curr.zoom = (abs(motion.zoom) > THRESHOLD);
	curr.roll = (abs(motion.roll) > THRESHOLD);
	curr.pan = (abs(motion.pan.u)+abs(motion.pan.v) > 2*THRESHOLD);
	curr.orbit = (abs(motion.orbit.u)+abs(motion.orbit.v) > 2*THRESHOLD);
}

/* These could probably be done more succinctly with some macro black magic
 * but that seems too scary for me now
 */


void apply_zoom( int zoom)
{
	if(curr.zoom)
	{
		bleCombo.press(KEY_LEFT_SHIFT);
		bleCombo.press(zoom>0 ? KEY_NUM_PLUS : KEY_NUM_MINUS);
		bleCombo.release(zoom>0 ? KEY_NUM_PLUS : KEY_NUM_MINUS);
		bleCombo.release(KEY_LEFT_SHIFT);
	}
}

void apply_roll( int roll)
{
	if(curr.roll)
	{
		bleCombo.press(KEY_LEFT_SHIFT);
		bleCombo.press(roll>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.release(roll>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.release(KEY_LEFT_SHIFT);
	}
}

void apply_orbit( struct uv orbit)
{
	if(curr.orbit)
	{
		bleCombo.press(orbit.u>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.release(orbit.u>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.press(orbit.v>0 ? KEY_NUM_8 : KEY_NUM_2);
		bleCombo.release(orbit.v>0 ? KEY_NUM_8 : KEY_NUM_2);
	}
}

void apply_pan( struct uv pan)
{
	if(curr.pan)
	{
		bleCombo.press(KEY_LEFT_CTRL);
		bleCombo.press(pan.u>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.release(pan.u>0 ? KEY_NUM_6 : KEY_NUM_4);
		bleCombo.press(pan.v>0 ? KEY_NUM_8 : KEY_NUM_2);
		bleCombo.release(pan.v>0 ? KEY_NUM_8 : KEY_NUM_2);
		bleCombo.release(KEY_LEFT_CTRL);
	}
}

void apply_motion()
{
	apply_zoom(motion.zoom);
	apply_roll(motion.roll);
//	apply_pan(motion.pan);
//	apply_orbit(motion.orbit);
}

void setup()
{
	pinMode(Pinout::BUTTON, INPUT_PULLUP);
	
	Serial.begin(9600);
	Serial.print("im not braindead!");
	for (int i=0;i<FILTERING;i++)
	{
		updateInput();
	}
	zero_all();
	bleCombo.begin();
}

void loop()
{
	updateInput();
//	plotInputs();
	calcMotion();
	plotMotions();
	if(enabled)
	{
		apply_motion();
	}
	 delay(DELAY);
}
