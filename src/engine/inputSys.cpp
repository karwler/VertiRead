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
	World::scene()->onMouseMove(vec2i(motion.x, motion.y), mouseMove);
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) {
	if (World::scene()->capture)	// different behaviour when capturing or not
		World::scene()->capture->onKeypress(key.keysym);
	else if (!key.repeat)			// handle only once pressed keys
		checkBindingsK(key.keysym.scancode);
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
	if (value == 0 || SDL_GameControllerFromInstanceID(jaxis.which))
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
	if (value == 0)
		return;

	if (World::scene()->capture)
		World::scene()->capture->onGAxis(SDL_GameControllerAxis(gaxis.axis), value > 0);
	else
		checkBindingsX(SDL_GameControllerAxis(gaxis.axis), value > 0);
}

void InputSys::tick(float) {
	// handle keyhold
	float amt = 1.f;
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [this, &amt](const Binding& bi) -> bool { return bi.isAxis() && isPressed(bi, amt); }); it != bindings.end())
		World::srun(it->getAcall(), amt);
}

void InputSys::checkBindingsK(SDL_Scancode key) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [key](const Binding& bi) -> bool { return !bi.isAxis() && bi.keyAssigned() && bi.getKey() == key; }); it != bindings.end())
		World::srun(it->getBcall());
}

void InputSys::checkBindingsB(uint8 jbutton) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [jbutton](const Binding& bi) -> bool { return !bi.isAxis() && bi.jbuttonAssigned() && bi.getJctID() == jbutton; }); it != bindings.end())
		World::srun(it->getBcall());
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [jhat, val](const Binding& bi) -> bool { return !bi.isAxis() && bi.jhatAssigned() && bi.getJctID() == jhat && bi.getJhatVal() == val; }); it != bindings.end())
		World::srun(it->getBcall());
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [jaxis, positive](const Binding& bi) -> bool { return !bi.isAxis() && ((bi.jposAxisAssigned() && positive) || (bi.jnegAxisAssigned() && !positive)) && bi.getJctID() == jaxis; }); it != bindings.end())
		World::srun(it->getBcall());
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [gbutton](const Binding& bi) -> bool { return !bi.isAxis() && bi.gbuttonAssigned() && bi.getGbutton() == gbutton; }); it != bindings.end())
		World::srun(it->getBcall());
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) {
	if (array<Binding, Binding::names.size()>::iterator it = std::find_if(bindings.begin(), bindings.end(), [gaxis, positive](const Binding& bi) -> bool { return !bi.isAxis() && ((bi.gposAxisAssigned() && positive) || (bi.gnegAxisAssigned() && !positive)) && bi.getGaxis() == gaxis; }); it != bindings.end())
		World::srun(it->getBcall());
}

bool InputSys::isPressed(const Binding& abind, float& amt) const {
	if (abind.keyAssigned() && SDL_GetKeyboardState(nullptr)[abind.getKey()])	// check keyboard keys
		return true;

	if (abind.jbuttonAssigned() && isPressedB(abind.getJctID()))	// check controller buttons
		return true;
	if (abind.jhatAssigned() && isPressedH(abind.getJctID(), abind.getJhatVal()))
		return true;
	if (abind.jaxisAssigned())	// check controller axes
		if (int val = getAxisJ(abind.getJctID()); (abind.jposAxisAssigned() && val > 0) || (abind.jnegAxisAssigned() && val < 0)) {
			amt = axisToFloat(abind.jposAxisAssigned() ? val : -val);
			return true;
		}

	if (abind.gbuttonAssigned() && isPressedG(abind.getGbutton()))	// check gamepad buttons
		return true;
	if (abind.gaxisAssigned())	// check controller axes
		if (int val = getAxisG(abind.getGaxis()); (abind.gposAxisAssigned() && val > 0) || (abind.gnegAxisAssigned() && val < 0)) {
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
			if (int val = checkAxisValue(SDL_JoystickGetAxis(it.joystick, jaxis)); val != 0)
				return val;
	return 0;
}

int InputSys::getAxisG(SDL_GameControllerAxis gaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (it.gamepad)
			if (int val = checkAxisValue(SDL_GameControllerGetAxis(it.gamepad, gaxis)); val != 0)
				return val;
	return 0;
}

void InputSys::resetBindings() {
	for (sizet i = 0; i < bindings.size(); i++)
		bindings[i].setDefaultSelf(Binding::Type(i));
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
