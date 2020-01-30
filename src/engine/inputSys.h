#pragma once

#include "utils/settings.h"

// handles input events and contains controls settings
class InputSys {
private:
	struct Controller {
		SDL_Joystick* joystick;			// for direct input
		SDL_GameController* gamepad;	// for xinput
		int index;

		Controller(int id);

		void close();
	};

	static constexpr uint32 moveTimeout = 50;

	vector<Controller> controllers;	// currently connected game controllers
	array<Binding, Binding::names.size()> bindings;
	vec2i mouseMove;				// last mouse motion
	uint32 moveTime;				// timestamp of last recorded mouseMove

public:
	InputSys();
	~InputSys();

	void eventMouseMotion(const SDL_MouseMotionEvent& motion);
	void eventMouseButtonDown(const SDL_MouseButtonEvent& button);
	void eventMouseButtonUp(const SDL_MouseButtonEvent& button);
	void eventKeypress(const SDL_KeyboardEvent& key);
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);

	void tick() const;
	bool isPressed(Binding::Type type, float& amt) const;	// looks through axis bindings (aka holders) in controls settings (amt will only be changed if the binding is an active axis)
	bool isPressed(const Binding& abind, float& amt) const;
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(SDL_GameControllerButton gbutton) const;	// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	int getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	int getAxisG(SDL_GameControllerAxis gaxis) const;			// check if any of the gamepads' axis value is greater than 0

	vec2i getMouseMove() const;
	Binding& getBinding(Binding::Type type);
	const array<Binding, Binding::names.size()>& getBindings() const;
	void resetBindings();
	void addController(int id);
	void removeController(int id);

private:
	void checkBindingsK(SDL_Scancode key, uint8 repeat) const;
	void checkBindingsB(uint8 jbutton) const;
	void checkBindingsH(uint8 jhat, uint8 val) const;
	void checkBindingsA(uint8 jaxis, bool positive) const;
	void checkBindingsG(SDL_GameControllerButton gbutton) const;
	void checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const;

	int checkAxisValue(int value) const;	// check deadzone in axis value
	static float axisToFloat(int axisValue);
};

inline bool InputSys::isPressed(Binding::Type type, float& amt) const {
	return bindings[sizet(type)].isHolder() ? isPressed(bindings[sizet(type)], amt) : false;
}

inline bool InputSys::isPressedB(uint8 jbutton) const {
	return std::find_if(controllers.begin(), controllers.end(), [jbutton](const Controller& it) -> bool { return !it.gamepad && SDL_JoystickGetButton(it.joystick, jbutton); }) != controllers.end();
}

inline bool InputSys::isPressedG(SDL_GameControllerButton gbutton) const {
	return std::find_if(controllers.begin(), controllers.end(), [gbutton](const Controller& it) -> bool { return !it.gamepad && SDL_GameControllerGetButton(it.gamepad, gbutton); }) != controllers.end();
}

inline vec2i InputSys::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : 0;
}

inline Binding& InputSys::getBinding(Binding::Type type) {
	return bindings[sizet(type)];
}

inline const array<Binding, Binding::names.size()>& InputSys::getBindings() const {
	return bindings;
}

inline float InputSys::axisToFloat(int axisValue) {
	return float(axisValue) / float(Settings::axisLimit);
}
