#include "world.h"

// CONTROLLER

bool InputSys::Controller::open(int id) {
	gamepad = SDL_IsGameController(id) ? SDL_GameControllerOpen(id) : nullptr;
	joystick = gamepad ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id);
	return gamepad || joystick;
}

void InputSys::Controller::close() {
	if (gamepad)
		SDL_GameControllerClose(gamepad);
	else if (joystick)
		SDL_JoystickClose(joystick);
}

// INPUT SYS

InputSys::InputSys() :
	mouseLast(false),
	bindings(World::fileSys()->getBindings()),
	mouseMove(0),
	moveTime(0)
{
	reloadControllers();
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion, bool mouse) {
	mouseLast = mouse;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	World::scene()->onMouseMove(ivec2(motion.x, motion.y), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button, bool mouse) {
	if (mouseLast = mouse; button.button < SDL_BUTTON_X1)
		World::scene()->onMouseDown(ivec2(button.x, button.y), button.button, button.clicks);
	else switch (button.button) {
	case SDL_BUTTON_X1:
		World::srun(bindings[uint8(Binding::Type::escape)].bcall);
		break;
	case SDL_BUTTON_X2:
		World::srun(bindings[uint8(Binding::Type::enter)].bcall);
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button, bool mouse) {
	if (mouseLast = mouse; button.button < SDL_BUTTON_X1)
		World::scene()->onMouseUp(ivec2(button.x, button.y), button.button, button.clicks);
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseLast = true;
	World::scene()->onMouseWheel(ivec2(wheel.x, -wheel.y));
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) {
	if (World::scene()->capture)	// different behavior when capturing or not
		World::scene()->capture->onKeypress(key.keysym);
	else
		checkBindingsK(key.keysym.scancode, key.repeat);
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJButton(jbutton.button);
	else
		checkBindingsB(jbutton.button);
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) {
	if (jhat.value == SDL_HAT_CENTERED || SDL_GameControllerFromInstanceID(jhat.which))
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJHat(jhat.hat, jhat.value);
	else
		checkBindingsH(jhat.hat, jhat.value);
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) {
	int value = checkAxisValue(jaxis.value);
	if (!value || SDL_GameControllerFromInstanceID(jaxis.which))
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJAxis(jaxis.axis, value > 0);
	else
		checkBindingsA(jaxis.axis, value > 0);
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->capture)
		World::scene()->capture->onGButton(SDL_GameControllerButton(gbutton.button));
	else
		checkBindingsG(SDL_GameControllerButton(gbutton.button));
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	int value = checkAxisValue(gaxis.value);
	if (!value)
		return;

	if (World::scene()->capture)
		World::scene()->capture->onGAxis(SDL_GameControllerAxis(gaxis.axis), value > 0);
	else
		checkBindingsX(SDL_GameControllerAxis(gaxis.axis), value > 0);
}

void InputSys::eventFingerMove(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::drawSys()->viewport().size();
	eventMouseMotion({ fin.type, fin.timestamp, World::winSys()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LMASK, int(fin.x * size.x), int(fin.y * size.y), int(fin.dx * size.x), int(fin.dy * size.y) }, false);
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::drawSys()->viewport().size());
	eventMouseButtonDown({ fin.type, fin.timestamp, World::winSys()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_PRESSED, 1, 0, pos.x, pos.y }, false);
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::drawSys()->viewport().size());
	eventMouseButtonUp({ fin.type, fin.timestamp, World::winSys()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_RELEASED, 1, 0, pos.x, pos.y }, false);
	World::scene()->updateSelect(ivec2(-1));
}

void InputSys::tick() const {
	// handle key hold
	for (sizet i = uint8(Binding::holders); i < bindings.size(); i++)
		if (float amt = 1.f; isPressed(bindings[i], amt))
			World::srun(bindings[i].acall, amt);
}

void InputSys::checkBindingsK(SDL_Scancode key, uint8 repeat) const {
	for (uint8 i = 0, e = uint8(repeat ? Binding::Type::right : Binding::holders); i < e; i++)
		if (bindings[i].keyAssigned() && bindings[i].getKey() == key)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsB(uint8 jbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); i++)
		if (bindings[i].jbuttonAssigned() && bindings[i].getJctID() == jbutton)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) const {
	for (uint8 i = 0; i < uint8(Binding::holders); i++)
		if (bindings[i].jhatAssigned() && bindings[i].getJctID() == jhat && bindings[i].getJhatVal() == val)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); i++)
		if (bindings[i].jposAxisAssigned() == positive && bindings[i].getJctID() == jaxis)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); i++)
		if (bindings[i].gbuttonAssigned() && bindings[i].getGbutton() == gbutton)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); i++)
		if (bindings[i].gposAxisAssigned() == positive && bindings[i].getGaxis() == gaxis)
			World::srun(bindings[i].bcall);
}

bool InputSys::isPressed(const Binding& abind, float& amt) const {
	if (abind.keyAssigned() && SDL_GetKeyboardState(nullptr)[abind.getKey()])	// check keyboard keys
		return true;

	if (abind.jbuttonAssigned() && isPressedB(abind.getJctID()))	// check controller buttons
		return true;
	if (abind.jhatAssigned() && isPressedH(abind.getJctID(), abind.getJhatVal()))
		return true;
	if (abind.jaxisAssigned())	// check controller axes
		if (int val = getAxisJ(abind.getJctID()); val && (val > 0 ? abind.jposAxisAssigned() : abind.jnegAxisAssigned())) {
			amt = axisToFloat(abind.jposAxisAssigned() ? val : -val);
			return true;
		}

	if (abind.gbuttonAssigned() && isPressedG(abind.getGbutton()))	// check gamepad buttons
		return true;
	if (abind.gaxisAssigned())	// check controller axes
		if (int val = getAxisG(abind.getGaxis()); val && (val > 0 ? abind.gposAxisAssigned() : abind.gnegAxisAssigned())) {
			amt = axisToFloat(abind.gposAxisAssigned() ? val : -val);
			return true;
		}
	return false;
}

bool InputSys::isPressedH(uint8 jhat, uint8 val) const {
	for (const Controller& it : controllers)
		if (!it.gamepad)
			for (int i = 0; i < SDL_JoystickNumHats(it.joystick); i++)
				if (jhat == i && SDL_JoystickGetHat(it.joystick, i) == val)
					return true;
	return false;
}

int InputSys::getAxisJ(uint8 jaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (!it.gamepad)
			if (int val = checkAxisValue(SDL_JoystickGetAxis(it.joystick, jaxis)); val)
				return val;
	return 0;
}

int InputSys::getAxisG(SDL_GameControllerAxis gaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (it.gamepad)
			if (int val = checkAxisValue(SDL_GameControllerGetAxis(it.gamepad, gaxis)); val)
				return val;
	return 0;
}

void InputSys::resetBindings() {
	for (sizet i = 0; i < bindings.size(); i++)
		bindings[i].reset(Binding::Type(i));
}

void InputSys::reloadControllers() {
	for (Controller& it : controllers)
		it.close();
	controllers.clear();

	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (Controller ctr; ctr.open(i))
			controllers.push_back(ctr);
		else
			ctr.close();
	}
}

int InputSys::checkAxisValue(int value) const {
	return std::abs(value) > World::sets()->getDeadzone() ? value : 0;
}

void InputSys::simulateMouseMove() {
	if (ivec2 pos; mouseLast) {
		uint32 state = SDL_GetMouseState(&pos.x, &pos.y);
		eventMouseMotion({ SDL_MOUSEMOTION, SDL_GetTicks(), World::winSys()->windowID(), 0, state, pos.x, pos.y, 0, 0 }, mouseLast);
	} else
		eventMouseMotion({ SDL_FINGERMOTION, SDL_GetTicks(), World::winSys()->windowID(), SDL_TOUCH_MOUSEID, 0, -1, -1, 0, 0 }, mouseLast);
}
