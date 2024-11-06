#pragma once

#include "utils/settings.h"
#include <SDL_events.h>
#include <SDL_timer.h>

// handles input events and contains controls settings
class InputSys {
private:
	struct Controller {
		SDL_GameController* gamepad;
		SDL_Joystick* joystick;

		Controller(SDL_GameController* gmp, SDL_Joystick* joy) noexcept : gamepad(gmp), joystick(joy) {}
	};

#if SDL_VERSION_ATLEAST(3, 2, 0)
	static constexpr uint64 moveTimeout = 50'000'000;
#else
	static constexpr uint32 moveTimeout = 50;
#endif

	vector<Controller> controllers;	// currently connected game controllers
	array<Binding, Binding::names.size()> bindings;
	tick_t moveTime = 0;			// timestamp of last recorded mouseMove
	vec2 mouseMove = vec2(0.f);		// last mouse motion
public:
	optional<uint32> mouseWin;		// last window id the mouse was in

	InputSys();
	~InputSys() { cleanup(); }

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
	bool isPressed(Binding::Type type, float& amt) const noexcept;	// looks through axis bindings (aka holders) in controls settings (amt will only be changed if the binding is an active axis)
	bool isPressed(const Binding& abind, float& amt) const noexcept;
	bool isPressedB(uint8 jbutton) const noexcept;	// check if any of the joysticks' button is pressed
	bool isPressedG(SDL_GameControllerButton gbutton) const noexcept;	// check if any of the gamepads' button is pressed
	bool isPressedH(uint8 jhat, uint8 val) const noexcept;	// check if any of the joysticks' hat is pressed
	int getAxisJ(uint8 jaxis) const noexcept;	// check if any of the joysticks' axis value is greater than 0
	int getAxisG(SDL_GameControllerAxis gaxis) const noexcept;	// check if any of the gamepads' axis value is greater than 0

	vec2 getMouseMove() const noexcept;
	Binding& getBinding(Binding::Type type) noexcept { return bindings[eint(type)]; }
	const array<Binding, Binding::names.size()>& getBindings() const noexcept { return bindings; }
	string getBoundName(Binding::Type type) const;
	void resetBindings() noexcept;
	void addJoystick(SDL_JoystickID jid);
	void addGamepad(SDL_JoystickID jid);
	void delJoystick(SDL_JoystickID jid);
	void delGamepad(SDL_JoystickID jid);
	void simulateMouseMove();

private:
	void cleanup() noexcept;

	void checkBindingsK(SDL_Scancode key, uint8 repeat) const;
	void checkBindingsB(uint8 jbutton) const;
	void checkBindingsH(uint8 jhat, uint8 val) const;
	void checkBindingsA(uint8 jaxis, bool positive) const;
	void checkBindingsG(SDL_GameControllerButton gbutton) const;
	void checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const;

	int checkAxisValue(int value) const noexcept;	// check deadzone in axis value
	static float axisToFloat(int axisValue) noexcept;
	static SDL_MouseButtonEvent toMouseEvent(const SDL_TouchFingerEvent& fin, bool down, vec2 winSize) noexcept;
};

inline bool InputSys::isPressed(Binding::Type type, float& amt) const noexcept {
	return type >= Binding::holders ? isPressed(bindings[eint(type)], amt) : false;
}

inline bool InputSys::isPressedB(uint8 jbutton) const noexcept {
	return rng::any_of(controllers, [jbutton](const Controller& it) -> bool { return !it.gamepad && SDL_JoystickGetButton(it.joystick, jbutton); });
}

inline bool InputSys::isPressedG(SDL_GameControllerButton gbutton) const noexcept {
	return rng::any_of(controllers, [gbutton](const Controller& it) -> bool { return !it.gamepad && SDL_GameControllerGetButton(it.gamepad, gbutton); });
}

inline vec2 InputSys::getMouseMove() const noexcept {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_GetTicksNS() - moveTime < moveTimeout ? mouseMove : vec2(0.f);
#else
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : vec2(0.f);
#endif
}

inline float InputSys::axisToFloat(int axisValue) noexcept {
	return float(axisValue) / float(Settings::axisLimit);
}
