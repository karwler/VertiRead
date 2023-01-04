#pragma once

#include "utils/settings.h"

// handles input events and contains controls settings
class InputSys {
private:
	struct Controller {
		SDL_GameController* gamepad;
		SDL_Joystick* joystick;

		bool open(int id);
		void close();
	};

	static constexpr uint32 moveTimeout = 50;

	vector<Controller> controllers;	// currently connected game controllers
	array<Binding, Binding::names.size()> bindings;
	ivec2 mouseMove = { 0, 0 };		// last mouse motion
	uint32 moveTime = 0;			// timestamp of last recorded mouseMove
public:
	optional<uint32> mouseWin;		// last windows id the mouse was in

	InputSys();
	~InputSys();

	void eventMouseMotion(const SDL_MouseMotionEvent& motion);
	void eventMouseButtonDown(const SDL_MouseButtonEvent& button);
	void eventMouseButtonUp(const SDL_MouseButtonEvent& button);
	void eventMouseWheel(const SDL_MouseWheelEvent& wheel);
	void eventKeypress(const SDL_KeyboardEvent& key) const;
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton) const;
	void eventJoystickHat(const SDL_JoyHatEvent& jhat) const;
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) const;
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) const;
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) const;
	void eventFingerMove(const SDL_TouchFingerEvent& fin);
	void eventFingerDown(const SDL_TouchFingerEvent& fin);
	void eventFingerUp(const SDL_TouchFingerEvent& fin);

	void tick() const;
	bool isPressed(Binding::Type type, float& amt) const;	// looks through axis bindings (aka holders) in controls settings (amt will only be changed if the binding is an active axis)
	bool isPressed(const Binding& abind, float& amt) const;
	bool isPressedB(uint8 jbutton) const;			// check if any of the joysticks' button is pressed
	bool isPressedG(SDL_GameControllerButton gbutton) const;	// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const;	// check if any of the joysticks' hat is pressed
	int getAxisJ(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	int getAxisG(SDL_GameControllerAxis gaxis) const;			// check if any of the gamepads' axis value is greater than 0

	ivec2 getMouseMove() const;
	Binding& getBinding(Binding::Type type);
	const array<Binding, Binding::names.size()>& getBindings() const;
	void resetBindings();
	void reloadControllers();
	void simulateMouseMove();

private:
	void checkBindingsK(SDL_Scancode key, uint8 repeat) const;
	void checkBindingsB(uint8 jbutton) const;
	void checkBindingsH(uint8 jhat, uint8 val) const;
	void checkBindingsA(uint8 jaxis, bool positive) const;
	void checkBindingsG(SDL_GameControllerButton gbutton) const;
	void checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const;

	int checkAxisValue(int value) const;	// check deadzone in axis value
	static float axisToFloat(int axisValue);
	static SDL_MouseButtonEvent toMouseEvent(const SDL_TouchFingerEvent& fin, uint8 state, vec2 winSize);
};

inline bool InputSys::isPressed(Binding::Type type, float& amt) const {
	return type >= Binding::holders ? isPressed(bindings[uint8(type)], amt) : false;
}

inline bool InputSys::isPressedB(uint8 jbutton) const {
	return std::find_if(controllers.begin(), controllers.end(), [jbutton](const Controller& it) -> bool { return !it.gamepad && SDL_JoystickGetButton(it.joystick, jbutton); }) != controllers.end();
}

inline bool InputSys::isPressedG(SDL_GameControllerButton gbutton) const {
	return std::find_if(controllers.begin(), controllers.end(), [gbutton](const Controller& it) -> bool { return !it.gamepad && SDL_GameControllerGetButton(it.gamepad, gbutton); }) != controllers.end();
}

inline ivec2 InputSys::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : ivec2(0);
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
