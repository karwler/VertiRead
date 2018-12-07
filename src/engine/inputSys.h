#pragma once

#include "utils/settings.h"

// handles input events and contains controls settings
class InputSys {
public:
	InputSys();
	~InputSys();

	void eventMouseMotion(const SDL_MouseMotionEvent& motion);
	void eventKeypress(const SDL_KeyboardEvent& key);
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);

	void tick(float);
	bool isPressed(Binding::Type type, float& amt) const;	// looks through axis bindings (aka holders) in controls settings (amt will only be changed if the binding is an active axis)
	bool isPressed(const Binding& abind, float& amt) const;
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(SDL_GameControllerButton gbutton) const;	// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	int getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	int getAxisG(SDL_GameControllerAxis gaxis) const;			// check if any of the gamepads' axis value is greater than 0

	const vec2i& getMouseMove() const;
	Binding& getBinding(Binding::Type type);
	const vector<Binding>& getBindings() const;
	void resetBindings();
	void addController(int id);
	void removeController(int id);

private:
	struct Controller {
		Controller(int id);

		void close();

		SDL_Joystick* joystick;			// for direct input
		SDL_GameController* gamepad;	// for xinput
		int index;
	};
	vector<Controller> controllers;	// currently connected game controllers
	vector<Binding> bindings;
	vec2i mouseMove;				// last mouse motion

	void checkBindingsK(SDL_Scancode key);
	void checkBindingsB(uint8 jbutton);
	void checkBindingsH(uint8 jhat, uint8 val);
	void checkBindingsA(uint8 jaxis, bool positive);
	void checkBindingsG(SDL_GameControllerButton gbutton);
	void checkBindingsX(SDL_GameControllerAxis gaxis, bool positive);

	int checkAxisValue(int value) const;	// check deadzone in axis value
	static float axisToFloat(int axisValue);
};

inline bool InputSys::isPressed(Binding::Type type, float& amt) const {
	return bindings[sizt(type)].isAxis() ? isPressed(bindings[sizt(type)], amt) : false;
}

inline const vec2i& InputSys::getMouseMove() const {
	return mouseMove;
}

inline Binding& InputSys::getBinding(Binding::Type type) {
	return bindings[sizt(type)];
}

inline const vector<Binding>& InputSys::getBindings() const {
	return bindings;
}

inline float InputSys::axisToFloat(int axisValue) {
	return float(axisValue) / float(Default::axisLimit);
}
