#include <string.h>
#include <Keyboard.h>
#include <Mouse.h>

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

#define sin60 (866/1000)
#define sin30 (1/2)

#define THRESHOLD 32
#define FILTERING 4
#define DELAY 10

enum Pinout:byte
{ // current pin configuration
	STICK0_U = A0,
	STICK0_V = A1,
	STICK1_U = A2,
	STICK1_V = A3,
	STICK2_U = A4,
	STICK2_V = A5,
};

int normA( int raw)
{ // centers the potentiometer values around 0
	return raw-512;
}

struct roll
{
	int val[FILTERING];
};


void rollroll( struct roll roller)
{
	memmove( &roller.val, &roller.val + sizeof(int), FILTERING - 1);
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
}

struct
{
	struct roll Au;
	struct roll Av;
	struct roll Bu;
	struct roll Bv;
	struct roll Cu;
	struct roll Cv;
}raw_rolling;

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
	rollroll(raw_rolling.Au);
	raw_rolling.Au.val[0] = normA(analogRead(Pinout::STICK0_V));
	
	rollroll(raw_rolling.Av);
	raw_rolling.Av.val[0] = normA(analogRead(Pinout::STICK0_U));
	
	rollroll(raw_rolling.Bu);
	raw_rolling.Bv.val[0] = normA(analogRead(Pinout::STICK0_V));
	
	rollroll(raw_rolling.Bv);
	raw_rolling.Bu.val[0] = normA(analogRead(Pinout::STICK0_U));
	
	rollroll(raw_rolling.Cu);
	raw_rolling.Cu.val[0] = normA(analogRead(Pinout::STICK0_V));
	
	rollroll(raw_rolling.Cv);
	raw_rolling.Cv.val[0] = normA(analogRead(Pinout::STICK0_U));
	
	input.A.v = roll_avg(raw_rolling.Au);
	input.A.u = roll_avg(raw_rolling.Av);
	input.B.v = roll_avg(raw_rolling.Bu);
	input.B.u = roll_avg(raw_rolling.Bv);
	input.C.v = roll_avg(raw_rolling.Cu);
	input.C.u = roll_avg(raw_rolling.Cv);
}


/*
 * THIS IS WHERE THE MAGIC HAPPENS
 * everythig else is mostly just boilerplate for reading values
 * and simulating keyboard&mouse inputs
 * This is where the kinematic expressions calculate
 * the movement on all channels of the 3D transform
 * from each stick's local UV space
 */
void getMotion()
{ // calculates the motions from the states of the joysticks
	motion.zoom = (input.A.v + input.B.v + input.C.v)/3;
	motion.roll = (input.A.u + input.B.u + input.C.u)/3;
	motion.pan = 
	{
		-input.A.u + sin30*(input.B.u + input.C.u), 
		sin60*(-input.B.u + input.C.u)
	};
	motion.orbit=
	{
		-sin60*(input.B.u - input.C.u), 
		-input.A.v + sin30*(input.B.v + input.C.v)
	};
	curr.zoom = (abs(motion.zoom) > THRESHOLD);
	curr.roll = (abs(motion.zoom) > THRESHOLD);
	curr.pan = (abs(motion.zoom) > THRESHOLD);
	curr.orbit = (abs(motion.zoom) > THRESHOLD);
}

/* These could probably be done more succinctly with some macro black magic
 * but that seems too scary for me now
 */


void blender_apply_zoom( int zoom)
{
	if(curr.zoom)
	{
		if(!prev.zoom)
		{ // rising edge
			Keyboard.press(KEY_LEFT_CTRL);
			delay(DELAY);
			Mouse.press(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(0, zoom);
		}
	}
	else
	{
		if(prev.zoom)
		{ // falling edge
			Keyboard.release(KEY_LEFT_CTRL);
			delay(DELAY);
			Mouse.release(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(0, zoom);
		}
	}
	prev.zoom = curr.zoom;
}

void blender_apply_roll( int roll)
{
	if(curr.roll)
	{
		if(!prev.roll)
		{ // rising edge
			/* you may have noticed these are empty :))
			this is because blender uses turntable rotation (Z always vertical).
			rolling would just make things more confusing
			*/
		}
		else
		{ // continous high
			
		}
	}
	else
	{
		if(prev.roll)
		{ // falling edge
			
		}
		else
		{ // continous high
			
		}
	}
	prev.roll = curr.roll;
}

void blender_apply_orbit( struct uv orbit)
{
	if(curr.orbit)
	{
		if(!prev.orbit)
		{ // rising edge
			Mouse.press(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(orbit.u, orbit.v);
		}
	}
	else
	{
		if(prev.orbit)
		{ // falling edge
			Mouse.release(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(orbit.u, orbit.v);
		}
	}
	prev.orbit = curr.orbit;
}

void blender_apply_pan( struct uv pan)
{
	if(curr.pan)
	{
		if(!prev.pan)
		{ // rising edge
			Mouse.press(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(pan.u, pan.v);
		}
	}
	else
	{
		if(prev.pan)
		{ // falling edge
			Mouse.release(MOUSE_MIDDLE);
			delay(DELAY);
		}
		else
		{ // continous high
			Mouse.move(pan.u, pan.v);
		}
	}
	prev.pan = curr.pan;
}

void blender_apply_motion()
{
	blender_apply_zoom(motion.zoom);
	blender_apply_roll(motion.roll);
	blender_apply_pan(motion.pan);
	blender_apply_orbit(motion.orbit);
}

void applyMotion()
{ // sends the required key&mouse inputs to apply desired motion
	
}

void setup()
{
	Serial.begin(9600);
	Keyboard.begin();
	Mouse.begin();
}

void loop()
{
	updateInput();
	getMotion();
	blender_apply_zoom(motion.zoom);
	//Serial.println(input.A.u);

	delay(10);
}
