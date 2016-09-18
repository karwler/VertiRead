#pragma once

#include "utils/settings.h"

class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());
	~InputSys();

	void Tick();
	void KeypressEvent(const SDL_KeyboardEvent& key);
	void ControllerButtonEvent(const SDL_JoyButtonEvent& jbutton);	// basically anything that isn't a hat or an axis
	void ControllerHatEvent(const SDL_JoyHatEvent& jhat);			// hat is equivalent to dpad
	void ControllerAxisEvent(const SDL_JoyAxisEvent& jaxis);		// one directionmight be mapped as multipe axes
	void MouseButtonDownEvent(const SDL_MouseButtonEvent& button);
	void MouseButtonUpEvent(const SDL_MouseButtonEvent& button);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);
	void TextEvent(const SDL_TextInputEvent& text);

	void CheckAxisShortcuts();
	bool isPressed(const string& holder, float* amt=nullptr) const;	// looks through axis shortcuts (aka holders) in controls settings (amt will only be changed if the shortcut is an active axis)
	static bool isPressedK(SDL_Scancode key);	// check if keyboard key is pressed
	static bool isPressedM(uint32 button);		// check if mouse button is pressed
	bool isPressedC(uint8 jbutton) const;		// check if any of the controllers' button is pressed
	float getAxis(uint8 axis) const;			// check if any of the controllers' axis value is bigger than 0
	static vec2i mousePos();					// get mouse poition
	vec2i mouseMove() const;					// get how much the mouse has moved since the last tick

	ControlsSettings Settings() const;
	void ScrollSpeed(const vec2f& sspeed);
	Shortcut* GetShortcut(const string& name);

	bool HasControllers() const;
	void UpdateControllers();

	Capturer* Captured() const;
	void SetCapture(Capturer* cbox);

private:
	ControlsSettings sets;
	vector<SDL_Joystick*> controllers;
	vec2i lastMousePos;
	Capturer* captured;

	void CheckShortcutsK(SDL_Scancode key);
	void CheckShortcutsB(uint8 jbutton);
	void CheckShortcutsH(uint8 jhat);
	void CheckShortcutsA(uint8 jaxis, bool positive);

	void ClearControllers();
};
