#pragma once

#include "utils/settings.h"

// handles input events and contains controls settings
class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());
	~InputSys();

	void eventKeypress(const SDL_KeyboardEvent& key);
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);
	void eventMouseMotion(const SDL_MouseMotionEvent& motion);
	void eventMouseButtonDown(const SDL_MouseButtonEvent& button);
	void eventMouseButtonUp(const SDL_MouseButtonEvent& button);
	void eventMouseWheel(const SDL_MouseWheelEvent& wheel);
	void eventText(const SDL_TextInputEvent& text);

	void checkAxisShortcuts();
	bool isPressed(const string& holder, float* amt=nullptr) const;	// looks through axis shortcuts (aka holders) in controls settings (amt will only be changed if the shortcut is an active axis)
	bool isPressed(const ShortcutAxis* sc, float* amt=nullptr) const;
	static bool isPressedK(SDL_Scancode key);		// check if keyboard key is pressed
	static bool isPressedM(uint32 button);			// check if mouse button is pressed
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(uint8 gbutton) const;			// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	float getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	float getAxisG(uint8 gaxis) const;				// check if any of the gamepads' axis value is greater than 0
	static vec2i mousePos();
	vec2i mosueMove() const;

	const ControlsSettings& getSettings() const;
	void setScrollSpeed(const vec2f& sspeed);
	void setDeadzone(int16 deadz);
	Shortcut* getShortcut(const string& name);

	bool hasControllers() const;
	void updateControllers();

	const Capturer* getCaptured() const;
	void setCaptured(Capturer* cbox);

private:
	ControlsSettings sets;
	vector<Controller> controllers;	// currently connected game controllers
	Capturer* captured;				// pointer to object currently hogging all keyboard input
	vec2i mMov;						// how much mouse has moved since last check

	void checkShortcutsK(SDL_Scancode key);
	void checkShortcutsB(uint8 jbutton);
	void checkShortcutsH(uint8 jhat, uint8 val);
	void checkShortcutsA(uint8 jaxis, bool positive);
	void checkShortcutsG(uint8 gbutton);
	void checkShortcutsX(uint8 gaxis, bool positive);

	void clearControllers();

	int16 checkAxisValue(int16 value) const;	// check deadzone in axis value
};
