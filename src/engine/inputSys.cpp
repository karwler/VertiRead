#include "inputSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "world.h"
#include "utils/widgets.h"

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
	bindings(World::fileSys()->getBindings())
{
	reloadControllers();
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion) {
	mouseLast = motion.type == SDL_MOUSEMOTION;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	World::scene()->onMouseMove(ivec2(motion.x, motion.y), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button) {
	mouseLast = button.type == SDL_MOUSEBUTTONDOWN;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_MIDDLE: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseDown(ivec2(button.x, button.y), button.button, button.clicks);
		break;
	case SDL_BUTTON_X1:
		World::srun(bindings[uint8(Binding::Type::escape)].bcall);
		break;
	case SDL_BUTTON_X2:
		World::srun(bindings[uint8(Binding::Type::enter)].bcall);
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button) {
	if (mouseLast = button.type == SDL_MOUSEBUTTONUP; button.button < SDL_BUTTON_X1)
		World::scene()->onMouseUp(ivec2(button.x, button.y), button.button, button.clicks);
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseLast = true;
	World::scene()->onMouseWheel(ivec2(wheel.x, -wheel.y));
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) const {
	if (World::scene()->getCapture())	// different behavior when capturing or not
		World::scene()->getCapture()->onKeypress(key.keysym);
	else
		checkBindingsK(key.keysym.scancode, key.repeat);
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) const {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJButton(jbutton.button);
	else
		checkBindingsB(jbutton.button);
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) const {
	if (jhat.value == SDL_HAT_CENTERED || SDL_GameControllerFromInstanceID(jhat.which))
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJHat(jhat.hat, jhat.value);
	else
		checkBindingsH(jhat.hat, jhat.value);
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) const {
	int value = checkAxisValue(jaxis.value);
	if (!value || SDL_GameControllerFromInstanceID(jaxis.which))
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJAxis(jaxis.axis, value > 0);
	else
		checkBindingsA(jaxis.axis, value > 0);
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) const {
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onGButton(SDL_GameControllerButton(gbutton.button));
	else
		checkBindingsG(SDL_GameControllerButton(gbutton.button));
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) const {
	int value = checkAxisValue(gaxis.value);
	if (!value)
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onGAxis(SDL_GameControllerAxis(gaxis.axis), value > 0);
	else
		checkBindingsX(SDL_GameControllerAxis(gaxis.axis), value > 0);
}

void InputSys::eventFingerMove(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::drawSys()->viewport().size();
	SDL_MouseMotionEvent event{};
	event.type = fin.type;
	event.timestamp = fin.timestamp;
	event.windowID = fin.windowID;
	event.which = SDL_TOUCH_MOUSEID;
	event.state = SDL_BUTTON_LMASK;
	event.x = int(fin.x * size.x);
	event.y = int(fin.y * size.y);
	event.x = int(fin.dx * size.x);
	event.y = int(fin.dy * size.y);
	eventMouseMotion(event);
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin) {
	eventMouseButtonDown(toMouseEvent(fin, SDL_PRESSED, World::drawSys()->viewport().size()));
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	eventMouseButtonUp(toMouseEvent(fin, SDL_RELEASED, World::drawSys()->viewport().size()));
	World::scene()->updateSelect(ivec2(-1));
}

SDL_MouseButtonEvent InputSys::toMouseEvent(const SDL_TouchFingerEvent& fin, uint8 state, vec2 winSize) {
	SDL_MouseButtonEvent event{};
	event.type = fin.type;
	event.timestamp = fin.timestamp;
	event.windowID = fin.windowID;
	event.which = SDL_TOUCH_MOUSEID;
	event.button = SDL_BUTTON_LEFT;
	event.state = state;
	event.clicks = 1;
	event.x = int(fin.x * winSize.x);
	event.y = int(fin.y * winSize.y);
	return event;
}

void InputSys::tick() const {
	// handle key hold
	for (sizet i = uint8(Binding::holders); i < bindings.size(); ++i)
		if (float amt = 1.f; isPressed(bindings[i], amt))
			World::srun(bindings[i].acall, amt);
}

void InputSys::checkBindingsK(SDL_Scancode key, uint8 repeat) const {
	for (uint8 i = 0, e = uint8(repeat ? Binding::Type::right : Binding::holders); i < e; ++i)
		if (bindings[i].keyAssigned() && bindings[i].getKey() == key)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsB(uint8 jbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jbuttonAssigned() && bindings[i].getJctID() == jbutton)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jhatAssigned() && bindings[i].getJctID() == jhat && bindings[i].getJhatVal() == val)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jposAxisAssigned() == positive && bindings[i].getJctID() == jaxis)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].gbuttonAssigned() && bindings[i].getGbutton() == gbutton)
			World::srun(bindings[i].bcall);
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
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
			for (int i = 0; i < SDL_JoystickNumHats(it.joystick); ++i)
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
	for (sizet i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));
}

void InputSys::reloadControllers() {
	for (Controller& it : controllers)
		it.close();
	controllers.clear();

	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
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
	SDL_MouseMotionEvent event{};
	event.timestamp = SDL_GetTicks();
	if (mouseLast) {
		event.type = SDL_MOUSEMOTION;
		event.state = SDL_GetMouseState(&event.x, &event.y);
	} else {
		event.type = SDL_FINGERMOTION;
		event.which = SDL_TOUCH_MOUSEID;
		event.x = INT_MIN;
		event.y = INT_MIN;
	}
	eventMouseMotion(event);
}
