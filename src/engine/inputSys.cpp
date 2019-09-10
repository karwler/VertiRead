#include "world.h"

// CONTROLLER

InputSys::Controller::Controller(int id) {
	gamepad = SDL_IsGameController(id) ? SDL_GameControllerOpen(id) : nullptr;
	joystick = gamepad ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id);
	index = joystick ? id : -1;
}

void InputSys::Controller::close() {
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = nullptr;
		joystick = nullptr;
	} else if (joystick) {
		SDL_JoystickClose(joystick);
		joystick = nullptr;
	}
}

// INPUT SYS

InputSys::InputSys() :
	bindings(World::fileSys()->getBindings()),
	mouseMove(0)
{
	for (int i = 0; i < SDL_NumJoysticks(); i++)
		addController(i);
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion) {
	mouseMove = vec2i(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	World::scene()->onMouseMove(vec2i(motion.x, motion.y), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button) {
	if (button.button < SDL_BUTTON_X1)
		World::scene()->onMouseDown(vec2i(button.x, button.y), button.button, button.clicks);
	else switch (button.button) {
	case SDL_BUTTON_X1:
		World::srun(bindings[uint8(Binding::Type::enter)].bcall);
		break;
	case SDL_BUTTON_X2:
		World::srun(bindings[uint8(Binding::Type::escape)].bcall);
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button) {
	if (button.button < SDL_BUTTON_X1)
		World::scene()->onMouseUp(vec2i(button.x, button.y), button.button, button.clicks);
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) {
	if (World::scene()->capture)	// different behaviour when capturing or not
		World::scene()->capture->onKeypress(key.keysym);
	else
		checkBindingsK(key.keysym.scancode, key.repeat);
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a gamecontroller event
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

void InputSys::tick() const {
	// handle keyhold
	float amt = 1.f;
	for (const Binding& it : bindings)
		if (it.isHolder() && isPressed(it, amt))
			World::srun(it.acall, amt);
}

void InputSys::checkBindingsK(SDL_Scancode key, uint8 repeat) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && it.keyAssigned() && it.getKey() == key && it.canRepeat() >= repeat)
			World::srun(it.bcall);
}

void InputSys::checkBindingsB(uint8 jbutton) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && it.jbuttonAssigned() && it.getJctID() == jbutton)
			World::srun(it.bcall);
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && it.jhatAssigned() && it.getJctID() == jhat && it.getJhatVal() == val)
			World::srun(it.bcall);
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && ((it.jposAxisAssigned() && positive) || (it.jnegAxisAssigned() && !positive)) && it.getJctID() == jaxis)
			World::srun(it.bcall);
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && it.gbuttonAssigned() && it.getGbutton() == gbutton)
			World::srun(it.bcall);
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) const {
	for (const Binding& it : bindings)
		if (!it.isHolder() && ((it.gposAxisAssigned() && positive) || (it.gnegAxisAssigned() && !positive)) && it.getGaxis() == gaxis)
			World::srun(it.bcall);
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
	for (uint8 i = 0; i < bindings.size(); i++)
		bindings[i].reset(Binding::Type(i));
}

void InputSys::addController(int id) {
	if (Controller ctr(id); ctr.index >= 0)
		controllers.push_back(ctr);
	else
		ctr.close();
}

void InputSys::removeController(int id) {
	if (vector<Controller>::iterator it = std::find_if(controllers.begin(), controllers.end(), [id](const Controller& ci) -> bool { return ci.index == id; }); it != controllers.end()) {
		it->close();
		controllers.erase(it);
	}
}

int InputSys::checkAxisValue(int value) const {
	return std::abs(value) > World::sets()->getDeadzone() ? value : 0;
}
