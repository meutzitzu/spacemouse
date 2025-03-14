#include <string.h>
#include "extrakeys.h"
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

#define sin60 1000/866
#define sin30 2/1

#define THRESHOLD 16
#define FILTERING 4
#define DELAY 8
#define SPEED 1/16

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
	raw_rolling.Au.val[0] =  normA(analogRead(Pinout::STICK0_U));
	                                                            
	rollroll(raw_rolling.Av);                                   
	raw_rolling.Av.val[0] =  normA(analogRead(Pinout::STICK0_V));
	                                                            
	rollroll(raw_rolling.Bu);                                   
	raw_rolling.Bv.val[0] =  normA(analogRead(Pinout::STICK1_V));
	                                                            
	rollroll(raw_rolling.Bv);                                   
	raw_rolling.Bu.val[0] =  normA(analogRead(Pinout::STICK1_U));
	                                                            
	rollroll(raw_rolling.Cu);                                   
	raw_rolling.Cu.val[0] =  normA(analogRead(Pinout::STICK2_U));
	                                                            
	rollroll(raw_rolling.Cv);                                   
	raw_rolling.Cv.val[0] =  normA(analogRead(Pinout::STICK2_V));
	
	input.A.v = roll_avg(raw_rolling.Au);
	input.A.u = roll_avg(raw_rolling.Av);
	input.B.v = roll_avg(raw_rolling.Bu);
	input.B.u = roll_avg(raw_rolling.Bv);
	input.C.v = roll_avg(raw_rolling.Cu);
	input.C.u = roll_avg(raw_rolling.Cv);
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
		Keyboard.press(KEY_LEFT_SHIFT);
		Keyboard.press(zoom>0 ? KEY_KEYPAD_PLUS : KEY_KEYPAD_MINUS);
		Keyboard.release(zoom>0 ? KEY_KEYPAD_PLUS : KEY_KEYPAD_MINUS);
		Keyboard.release(KEY_LEFT_SHIFT);
	}
}

void apply_roll( int roll)
{
	if(curr.roll)
	{
		Keyboard.press(KEY_LEFT_SHIFT);
		Keyboard.press(roll>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.release(roll>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.release(KEY_LEFT_SHIFT);
	}
}

void apply_orbit( struct uv orbit)
{
	if(curr.orbit)
	{
		Keyboard.press(orbit.u>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.release(orbit.u>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.press(orbit.v>0 ? KEY_KEYPAD_8 : KEY_KEYPAD_2);
		Keyboard.release(orbit.v>0 ? KEY_KEYPAD_8 : KEY_KEYPAD_2);
	}
}

void apply_pan( struct uv pan)
{
	if(curr.pan)
	{
		Keyboard.press(KEY_LEFT_CTRL);
		Keyboard.press(pan.u>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.release(pan.u>0 ? KEY_KEYPAD_6 : KEY_KEYPAD_4);
		Keyboard.press(pan.v>0 ? KEY_KEYPAD_8 : KEY_KEYPAD_2);
		Keyboard.release(pan.v>0 ? KEY_KEYPAD_8 : KEY_KEYPAD_2);
		Keyboard.release(KEY_LEFT_CTRL);
	}
}

void apply_motion()
{
	//apply_zoom(motion.zoom);
	apply_roll(motion.roll);
	apply_pan(motion.pan);
	apply_orbit(motion.orbit);
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
	//printInputs();
	//printMotions();
	apply_motion();
	delay(DELAY);
}
