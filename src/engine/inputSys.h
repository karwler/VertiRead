#pragma once

#include "utils/settings.h"

// handles input events and contains controls settings
class InputSys {
public:
	InputSys();
	~InputSys();

	void eventKeypress(const SDL_KeyboardEvent& key);
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);
	void eventMouseMotion(const SDL_MouseMotionEvent& motion);

	void tick(float dSec);
	bool isPressed(Binding::Type type, float& amt) const;	// looks through axis bindings (aka holders) in controls settings (amt will only be changed if the binding is an active axis)
	bool isPressed(const Binding& abind, float& amt) const;
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(SDL_GameControllerButton gbutton) const;			// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	int getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	int getAxisG(SDL_GameControllerAxis gaxis) const;				// check if any of the gamepads' axis value is greater than 0

	static vec2i mousePos();
	const vec2i& getMouseMove() { return mouseMove; }
	Binding& getBinding(Binding::Type type) { return bindings[static_cast<sizt>(type)]; }
	const vector<Binding>& getBindings() const { return bindings; }
	void addController(int id);
	void removeController(int id);

private:
	struct Controller {
		Controller(int id);

		bool open(int id);
		void close();

		SDL_Joystick* joystick;			// for direct input
		SDL_GameController* gamepad;	// for xinput
		int index;
	};
	vector<Controller> controllers;	// currently connected game controllers
	vector<Binding> bindings;
	vec2i mouseMove;				// how much mouse has moved since last check

	void checkBindingsK(SDL_Scancode key);
	void checkBindingsB(uint8 jbutton);
	void checkBindingsH(uint8 jhat, uint8 val);
	void checkBindingsA(uint8 jaxis, bool positive);
	void checkBindingsG(uint8 gbutton);
	void checkBindingsX(uint8 gaxis, bool positive);

	void clearControllers();

	int checkAxisValue(int value) const;	// check deadzone in axis value
};
