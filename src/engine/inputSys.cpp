#include "inputSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "world.h"
#include "prog/program.h"
#include "prog/progs.h"
#include "utils/widgets.h"

bool Controller::axisDown(uint8 id, int16 val) noexcept {
	if (id < 64) {
		uint64 bit = 1 << id;
		if (bool cur = val >= axisThreshold; bool(axes & bit) != cur) {
			axes = cur ? axes | bit : axes & ~bit;
			return cur;
		}
	}
	return false;
}

uint8 Joystick::hatDown(uint8 id, int16 val) noexcept {
	if (id < 16) {
		uint8 shift = id * 4;
		uint8 pressed = ~(hats >> shift) & val;	// no need for & 0xF since val shouldn't be higher
		hats = (hats & ~(0xF << shift)) | (val << shift);
		return pressed;
	}
	return SDL_HAT_CENTERED;
}

InputSys::InputSys() :
	bindings(World::fileSys()->loadBindings())
{
#ifdef WITH_SDL3
	int cnt;
	if (uptr<SDL_JoystickID[], SdlFreePtr> jids(SDL_GetJoysticks(&cnt)); jids)	// TODO: is this necessary or will events be generated
		for (int i = 0; i < cnt; ++i) {
			if (SDL_IsGameController(jids[i]))
				addGamepad(jids[i]);
			else
				addJoystick(jids[i]);
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
	for (auto& [jid, joy] : joysticks)
		SDL_JoystickClose(joy.getCtr());
	for (auto& [jid, pad] : gamepads)
		SDL_GameControllerClose(pad.getCtr());
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion) {
	mouseWin = motion.type == SDL_MOUSEMOTION ? optional(motion.windowID) : std::nullopt;
	mouseMove = vec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	World::scene()->onMouseMove(ivec2(motion.x, motion.y) + World::winSys()->winViewOffset(motion.windowID), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button) {
	if (button.type == SDL_MOUSEBUTTONDOWN) {
		mouseWin = button.windowID;
		lastDevice = Binding::Device::keyboard;
	} else
		mouseWin = std::nullopt;

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
	lastDevice = Binding::Device::keyboard;
	World::scene()->onMouseWheel(vec2(wheel.x, -wheel.y));
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) {
	lastDevice = Binding::Device::keyboard;
#ifdef WITH_SDL3
	if (World::scene()->getCapture())	// different behavior when capturing or not
		World::scene()->getCapture()->onKeypress(key.key, key.mod);
	else
		checkBindingsK(key.key, key.repeat);
#else
	if (World::scene()->getCapture())	// different behavior when capturing or not
		World::scene()->getCapture()->onKeypress(key.keysym.sym, SDL_Keymod(key.keysym.mod));
	else
		checkBindingsK(key.keysym.sym, key.repeat);
#endif
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event	// TODO: is this necessary?
		return;

	lastDevice = Binding::Device::joystick;
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJButton(jbutton.button);
	else
		checkBindingsB(jbutton.button);
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) {
	auto joy = joysticks.find(jhat.which);
	if (joy == joysticks.end())
		return;
	uint8 value = joy->second.hatDown(jhat.hat, jhat.value);
	if (value == SDL_HAT_CENTERED)
		return;

	lastDevice = Binding::Device::joystick;
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJHat(jhat.hat, value);
	else
		checkBindingsH(jhat.hat, value);
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) {
	auto joy = joysticks.find(jaxis.which);
	if (joy == joysticks.end())
		return;
	int16 value = checkAxisValue(jaxis.value);
	if (!joy->second.axisDown(jaxis.axis, value))
		return;

	lastDevice = Binding::Device::joystick;
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJAxis(jaxis.axis, value > 0);
	else
		checkBindingsA(jaxis.axis, value > 0);
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	lastDevice = Binding::Device::gamepad;
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onGButton(SDL_GameControllerButton(gbutton.button));
	else
		checkBindingsG(SDL_GameControllerButton(gbutton.button));
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	auto pad = gamepads.find(gaxis.which);
	if (pad == gamepads.end())
		return;
	int16 value = checkAxisValue(gaxis.value);
	if (!pad->second.axisDown(gaxis.axis, value))
		return;

	lastDevice = Binding::Device::gamepad;
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
#ifdef WITH_SDL3
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
#ifdef WITH_SDL3
		.down = down,
#else
		.state = uint8(down ? SDL_PRESSED : SDL_RELEASED),
#endif
		.clicks = 1,
#ifdef WITH_SDL3
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
	for (uint8 i = eint(Binding::holders); i < bindings.size(); ++i)
		if (float amt = 1.f; isPressed(bindings[i], amt))
			World::program()->getState()->exec(bindings[i].acall, amt);
}

void InputSys::checkBindingsK(SDL_Keycode key, uint8 repeat) const {
	for (uint8 i = 0, e = eint(repeat ? Binding::Type::right : Binding::holders); i < e; ++i)
		if (bindings[i].keyAssigned() && bindings[i].getKey() == key)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsB(uint8 jbutton) const {
	for (uint8 i = 0; i < eint(Binding::holders); ++i)
		if (bindings[i].jbuttonAssigned() && bindings[i].getJctID() == jbutton)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) const {
	for (uint8 i = 0; i < eint(Binding::holders); ++i)
		if (bindings[i].jhatAssigned() && bindings[i].getJctID() == jhat && bindings[i].getJhatVal() == val)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) const {
	for (uint8 i = 0; i < eint(Binding::holders); ++i)
		if (bindings[i].jposAxisAssigned() == positive && bindings[i].getJctID() == jaxis)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) const {
	for (uint8 i = 0; i < eint(Binding::holders); ++i)
		if (bindings[i].gbuttonAssigned() && bindings[i].getGbutton() == gbutton)
			World::program()->getState()->exec(bindings[i].bcall);
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const {
	for (uint8 i = 0; i < eint(Binding::holders); ++i)
		if (bindings[i].gposAxisAssigned() == positive && bindings[i].getGaxis() == gaxis)
			World::program()->getState()->exec(bindings[i].bcall);
}

bool InputSys::isPressed(const Binding& abind, float& amt) const noexcept {
	if (abind.keyAssigned() && SDL_GetKeyboardState(nullptr)[scancodeFromKey(abind.getKey())])	// check keyboard keys
		return true;

	if (abind.jbuttonAssigned() && isPressedB(abind.getJctID()))	// check controller buttons
		return true;
	if (abind.jhatAssigned() && isPressedH(abind.getJctID(), abind.getJhatVal()))
		return true;
	if (abind.jaxisAssigned())	// check controller axes
		if (int16 val = getAxisJ(abind.getJctID()); val && (val > 0 ? abind.jposAxisAssigned() : abind.jnegAxisAssigned())) {
			amt = axisToFloat(abind.jposAxisAssigned() ? val : -val);
			return true;
		}

	if (abind.gbuttonAssigned() && isPressedG(abind.getGbutton()))	// check gamepad buttons
		return true;
	if (abind.gaxisAssigned())	// check controller axes
		if (int16 val = getAxisG(abind.getGaxis()); val && (val > 0 ? abind.gposAxisAssigned() : abind.gnegAxisAssigned())) {
			amt = axisToFloat(abind.gposAxisAssigned() ? val : -val);
			return true;
		}
	return false;
}

bool InputSys::isPressedH(uint8 jhat, uint8 val) const noexcept {
	for (auto& [jid, joy] : joysticks)
		for (int i = 0; i < SDL_JoystickNumHats(joy.getCtr()); ++i)
			if (jhat == i && SDL_JoystickGetHat(joy.getCtr(), i) == val)
				return true;
	return false;
}

int16 InputSys::getAxisJ(uint8 jaxis) const noexcept {
	for (auto& [jid, joy] : joysticks)	// get first axis that isn't 0
		if (int16 val = checkAxisValue(SDL_JoystickGetAxis(joy.getCtr(), jaxis)); val)
			return val;
	return 0;
}

int16 InputSys::getAxisG(SDL_GameControllerAxis gaxis) const noexcept {
	for (auto& [jid, pad] : gamepads)	// get first axis that isn't 0
		if (int16 val = checkAxisValue(SDL_GameControllerGetAxis(pad.getCtr(), gaxis)); val)
			return val;
	return 0;
}

string InputSys::getBoundName(Binding::Type type) const {
	const Binding& bind = bindings[eint(type)];
	switch (lastDevice) {
	using enum Binding::Device;
	case joystick:
		if (bind.jbuttonAssigned())
			return toStr(bind.getJctID());
		if (bind.jhatAssigned())
			return fmt::format("{:d} {}", bind.getJctID(), Binding::hatValueToName(bind.getJhatVal()));
		if (bind.jaxisAssigned())
			return fmt::format("{}{:d}", bind.jposAxisAssigned() ? '+' : '-', bind.getJctID());
		break;
	case gamepad:
		if (bind.gbuttonAssigned())
			return Binding::gbuttonNames[eint(bind.getGbutton())];
		if (bind.gaxisAssigned())
			return fmt::format("{}{}", bind.gposAxisAssigned() ? '+' : '-', Binding::gaxisNames[eint(bind.getGaxis())]);
	}
	return bind.keyAssigned() ? SDL_GetKeyName(bind.getKey()) : string();
}

void InputSys::resetBindings() noexcept {
	for (size_t i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));
}

void InputSys::addJoystick(SDL_JoystickID jid) {
	if (Joystick joy(jid); joy.getCtr())
		if (auto [it, ok] = joysticks.emplace(jid, joy); !ok) {
			SDL_JoystickClose(it->second.getCtr());
			it->second = joy;
		}
}

void InputSys::addGamepad(SDL_JoystickID jid) {
	if (Gamepad pad(jid); pad.getCtr())
		if (auto [it, ok] = gamepads.emplace(jid, pad); !ok) {
			SDL_GameControllerClose(it->second.getCtr());
			it->second = pad;
		}
}

void InputSys::delJoystick(SDL_JoystickID jid) {
	if (auto it = joysticks.find(jid); it != joysticks.end()) {
		SDL_JoystickClose(it->second.getCtr());
		joysticks.erase(it);
	}
}

void InputSys::delGamepad(SDL_JoystickID jid) {
	if (auto it = gamepads.find(jid); it != gamepads.end()) {
		SDL_GameControllerClose(it->second.getCtr());
		gamepads.erase(it);
	}
}

int16 InputSys::checkAxisValue(int16 value) const noexcept {
	return std::abs(int(value)) > int(World::sets()->getDeadzone()) ? value : 0;
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
