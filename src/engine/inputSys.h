#pragma once

#include "utils/settings.h"

class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());
	~InputSys();

	void KeypressEvent(const SDL_KeyboardEvent& key);
	void JoystickButtonEvent(const SDL_JoyButtonEvent& jbutton);	// basically anything that isn't a hat or an axis
	void JoystickHatEvent(const SDL_JoyHatEvent& jhat);				// hat is equivalent to dpad
	void JoystickAxisEvent(const SDL_JoyAxisEvent& jaxis);			// one directionmight be mapped as multipe axes
	void GamepadButtonEvent(const SDL_ControllerButtonEvent& gbutton);
	void GamepadAxisEvent(const SDL_ControllerAxisEvent& gaxis);
	void MouseMotionEvent(const SDL_MouseMotionEvent& motion);
	void MouseButtonDownEvent(const SDL_MouseButtonEvent& button);
	void MouseButtonUpEvent(const SDL_MouseButtonEvent& button);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);
	void TextEvent(const SDL_TextInputEvent& text);

	void CheckAxisShortcuts();
	bool isPressed(const string& holder, float* amt=nullptr) const;	// looks through axis shortcuts (aka holders) in controls settings (amt will only be changed if the shortcut is an active axis)
	bool isPressed(const ShortcutAxis* sc, float* amt=nullptr) const;
	static bool isPressedK(SDL_Scancode key);		// check if keyboard key is pressed
	static bool isPressedM(uint32 button);			// check if mouse button is pressed
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(uint8 gbutton) const;			// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	float getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	float getAxisG(uint8 gaxis) const;				// check if any of the gamepads' axis value is greater than 0
	vec2i mousePos() const;							// get mouse poition

	const ControlsSettings& Settings() const;
	void ScrollSpeed(const vec2f& sspeed);
	void Deadzone(int16 deadz);
	Shortcut* GetShortcut(const string& name);

	bool HasControllers() const;
	void UpdateControllers();

	const Capturer* Captured() const;
	void SetCapture(Capturer* cbox);

private:
	ControlsSettings sets;
	vector<Controller> controllers;
	vec2i mPos;
	Capturer* captured;

	void CheckShortcutsK(SDL_Scancode key);
	void CheckShortcutsB(uint8 jbutton);
	void CheckShortcutsH(uint8 jhat, uint8 val);
	void CheckShortcutsA(uint8 jaxis, bool positive);
	void CheckShortcutsG(uint8 gbutton);
	void CheckShortcutsX(uint8 gaxis, bool positive);

	void ClearControllers();

	int16 CheckAxisValue(int16 value) const;	// check deadzone in axis value
};
