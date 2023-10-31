#include "inputSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "world.h"
#include "prog/program.h"
#include "prog/progs.h"
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
	bindings(World::fileSys()->loadBindings())
{
	reloadControllers();
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion) {
	mouseWin = motion.type == SDL_MOUSEMOTION ? optional(motion.windowID) : std::nullopt;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	World::scene()->onMouseMove(ivec2(motion.x, motion.y) + World::winSys()->winViewOffset(motion.windowID), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button) {
	mouseWin = button.type == SDL_MOUSEBUTTONDOWN ? optional(button.windowID) : std::nullopt;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_MIDDLE: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseDown(ivec2(button.x, button.y) + World::winSys()->winViewOffset(button.windowID), button.button, button.clicks);
		break;
	case SDL_BUTTON_X1:
		World::program()->getState()->exec(bindings[eint(Binding::Type::escape)].bcall);
		break;
	case SDL_BUTTON_X2:
		World::program()->getState()->exec(bindings[eint(Binding::Type::enter)].bcall);
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button) {
	mouseWin = button.type == SDL_MOUSEBUTTONUP ? optional(button.windowID) : std::nullopt;
	if (button.button < SDL_BUTTON_X1)
		World::scene()->onMouseUp(ivec2(button.x, button.y) + World::winSys()->winViewOffset(button.windowID), button.button, button.clicks);
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseWin = wheel.windowID;
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
	vec2 size = World::drawSys()->getViewRes();
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
	eventMouseButtonDown(toMouseEvent(fin, SDL_PRESSED, World::drawSys()->getViewRes()));
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	eventMouseButtonUp(toMouseEvent(fin, SDL_RELEASED, World::drawSys()->getViewRes()));
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
	for (size_t i = uint8(Binding::holders); i < bindings.size(); ++i)
		if (float amt = 1.f; isPressed(bindings[i], amt))
			World::program()->getState()->exec(bindings[i].acall, amt);
}

void InputSys::checkBindingsK(SDL_Scancode key, uint8 repeat) const {
	for (uint8 i = 0, e = uint8(repeat ? Binding::Type::right : Binding::holders); i < e; ++i)
		if (bindings[i].keyAssigned() && bindings[i].getKey() == key)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsB(uint8 jbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jbuttonAssigned() && bindings[i].getJctID() == jbutton)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jhatAssigned() && bindings[i].getJctID() == jhat && bindings[i].getJhatVal() == val)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].jposAxisAssigned() == positive && bindings[i].getJctID() == jaxis)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].gbuttonAssigned() && bindings[i].getGbutton() == gbutton)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const {
	for (uint8 i = 0; i < uint8(Binding::holders); ++i)
		if (bindings[i].gposAxisAssigned() == positive && bindings[i].getGaxis() == gaxis)
			World::program()->getState()->exec(bindings[i].bcall);
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

string InputSys::getBoundName(Binding::Type type) const {
	const Binding& bind = bindings[eint(type)];
	if (bind.keyAssigned())
		return SDL_GetScancodeName(bind.getKey());
	if (bind.jbuttonAssigned())
		return toStr(bind.getJctID());
	if (bind.jhatAssigned())
		return std::format("{:d} {}", bind.getJctID(), Binding::hatNames.at(bind.getJhatVal()));
	if (bind.jaxisAssigned())
		return std::format("{}{:d}", bind.jposAxisAssigned() ? '+' : '-', bind.getJctID());
	if (bind.gbuttonAssigned())
		return Binding::gbuttonNames[eint(bind.getGbutton())];
	if (bind.gbuttonAssigned())
		return std::format("{}{}", bind.gposAxisAssigned() ? '+' : '-', Binding::gaxisNames[eint(bind.getGaxis())]);
	return string();
}

void InputSys::resetBindings() {
	for (size_t i = 0; i < bindings.size(); ++i)
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
	if (mouseWin) {
		event.type = SDL_MOUSEMOTION;
		event.windowID = *mouseWin;
		event.state = SDL_GetMouseState(&event.x, &event.y);
	} else {
		event.type = SDL_FINGERMOTION;
		event.which = SDL_TOUCH_MOUSEID;
		event.x = INT_MIN;
		event.y = INT_MIN;
	}
	eventMouseMotion(event);
}
