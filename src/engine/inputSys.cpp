#include "inputSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "world.h"
#include "prog/program.h"
#include "prog/progs.h"
#include "utils/widgets.h"

InputSys::InputSys() :
	bindings(World::fileSys()->loadBindings())
{
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (int cnt; SDL_JoystickID* jids = SDL_GetJoysticks(&cnt)) {
		for (int i = 0; i < cnt; ++i) {
			if (SDL_IsGameController(jids[i]))
				addGamepad(jids[i]);
			else
				addJoystick(jids[i]);
		}
		SDL_free(jids);
	}
#else
	for (int i = 0, e = SDL_NumJoysticks(); i < e; ++i) {
		if (SDL_IsGameController(i))
			addGamepad(i);
		else
			addJoystick(i);
	}
#endif
}

void InputSys::cleanup() noexcept {
	for (Controller& it : controllers) {
		if (it.gamepad)
			SDL_GameControllerClose(it.gamepad);
		else
			SDL_JoystickClose(it.joystick);
	}
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
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (World::scene()->getCapture())	// different behavior when capturing or not
		World::scene()->getCapture()->onKeypress(key.scancode, key.mod);
	else
		checkBindingsK(key.scancode, key.repeat);
#else
	if (World::scene()->getCapture())	// different behavior when capturing or not
		World::scene()->getCapture()->onKeypress(key.keysym.scancode, SDL_Keymod(key.keysym.mod));
	else
		checkBindingsK(key.keysym.scancode, key.repeat);
#endif
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
	eventMouseMotion({
		.type = fin.type,
		.timestamp = fin.timestamp,
		.windowID = fin.windowID,
		.which = SDL_TOUCH_MOUSEID,
		.state = SDL_BUTTON_LMASK,
#if SDL_VERSION_ATLEAST(3, 0, 0)
		.x = fin.x * size.x,
		.y = fin.y * size.y,
		.xrel = fin.dx * size.x,
		.yrel = fin.dy * size.y
#else
		.x = int(fin.x * size.x),
		.y = int(fin.y * size.y),
		.xrel = int(fin.dx * size.x),
		.yrel = int(fin.dy * size.y)
#endif
	});
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin) {
	eventMouseButtonDown(toMouseEvent(fin, true, World::drawSys()->getViewRes()));
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	eventMouseButtonUp(toMouseEvent(fin, false, World::drawSys()->getViewRes()));
	World::scene()->deselect();
}

SDL_MouseButtonEvent InputSys::toMouseEvent(const SDL_TouchFingerEvent& fin, bool down, vec2 winSize) noexcept {
	return {
		.type = fin.type,
		.timestamp = fin.timestamp,
		.windowID = fin.windowID,
		.which = SDL_TOUCH_MOUSEID,
		.button = SDL_BUTTON_LEFT,
#if SDL_VERSION_ATLEAST(3, 0, 0)
		.down = down,
#else
		.state = uint8(down ? SDL_PRESSED : SDL_RELEASED),
#endif
		.clicks = 1,
#if SDL_VERSION_ATLEAST(3, 0, 0)
		.x = fin.x * winSize.x,
		.y = fin.y * winSize.y
#else
		.x = int(fin.x * winSize.x),
		.y = int(fin.y * winSize.y)
#endif
	};
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
		return std::format("{:d} {}", bind.getJctID(), Binding::hatValueToName(bind.getJhatVal()));
	if (bind.jaxisAssigned())
		return std::format("{}{:d}", bind.jposAxisAssigned() ? '+' : '-', bind.getJctID());
	if (bind.gbuttonAssigned())
		return Binding::gbuttonNames[eint(bind.getGbutton())];
	if (bind.gaxisAssigned())
		return std::format("{}{}", bind.gposAxisAssigned() ? '+' : '-', Binding::gaxisNames[eint(bind.getGaxis())]);
	return string();
}

void InputSys::resetBindings() {
	for (size_t i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));
}

void InputSys::addJoystick(SDL_JoystickID jid) {
	if (SDL_Joystick* joystick = SDL_JoystickOpen(jid))
		controllers.emplace_back(nullptr, joystick);
}

void InputSys::addGamepad(SDL_JoystickID jid) {
	if (SDL_GameController* gamepad = SDL_GameControllerOpen(jid))
		controllers.emplace_back(gamepad, SDL_GameControllerGetJoystick(gamepad));
}

void InputSys::delJoystick(SDL_JoystickID jid) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (vector<Controller>::iterator cit = rng::find_if(controllers, [jid](const Controller& it) -> bool { return !it.gamepad && SDL_GetJoystickID(it.joystick) == jid; }); cit != controllers.end()) {
#else
	if (vector<Controller>::iterator cit = rng::find_if(controllers, [jid](const Controller& it) -> bool { return !it.gamepad && SDL_JoystickInstanceID(it.joystick) == jid; }); cit != controllers.end()) {
#endif
		SDL_JoystickClose(cit->joystick);
		controllers.erase(cit);
	}
}

void InputSys::delGamepad(SDL_JoystickID jid) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (vector<Controller>::iterator cit = rng::find_if(controllers, [jid](const Controller& it) -> bool { return it.gamepad && SDL_GetGamepadID(it.gamepad) == jid; }); cit != controllers.end()) {
#else
	if (vector<Controller>::iterator cit = rng::find_if(controllers, [jid](const Controller& it) -> bool { return it.gamepad && it.joystick && SDL_JoystickInstanceID(it.joystick) == jid; }); cit != controllers.end()) {
#endif
		SDL_GameControllerClose(cit->gamepad);
		controllers.erase(cit);
	}
}

int InputSys::checkAxisValue(int value) const {
	return std::abs(value) > World::sets()->getDeadzone() ? value : 0;
}

void InputSys::simulateMouseMove() {
	SDL_MouseMotionEvent event = { .timestamp = SDL_GetTicks() };
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
