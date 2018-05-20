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
	bindings(Filer::getBindings())
{
	for (int i=0; i<SDL_NumJoysticks(); i++)
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
	// different behaviour when capturing or not
	if (World::scene()->capture)
		World::scene()->capture->onKeypress(key.keysym);
	else if (!key.repeat)	// handle only once pressed keys
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
		World::scene()->capture->onJAxis(jaxis.axis, (value > 0));
	else
		checkBindingsA(jaxis.axis, (value > 0));
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->capture)
		World::scene()->capture->onGButton(static_cast<SDL_GameControllerButton>(gbutton.button));
	else
		checkBindingsG(static_cast<SDL_GameControllerButton>(gbutton.button));
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	int value = checkAxisValue(gaxis.value);
	if (value == 0)
		return;

	if (World::scene()->capture)
		World::scene()->capture->onGAxis(static_cast<SDL_GameControllerAxis>(gaxis.axis), (value > 0));
	else
		checkBindingsX(static_cast<SDL_GameControllerAxis>(gaxis.axis), (value > 0));
}

void InputSys::tick(float dSec) {
	// handle keyhold
	for (Binding& it : bindings)
		if (it.isAxis()) {
			float amt = 1.f;
			if (isPressed(it, amt) && it.getAcall())
				(World::state()->*it.getAcall())(amt);
		}
}

void InputSys::checkBindingsK(SDL_Scancode key) {
	for (Binding& it : bindings)	// find first binding with this key assigned to it
		if (!it.isAxis() && it.keyAssigned() && it.getKey() == key && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

void InputSys::checkBindingsB(uint8 jbutton) {
	for (Binding& it : bindings)
		if (!it.isAxis() && it.jbuttonAssigned() && it.getJctID() == jbutton && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

void InputSys::checkBindingsH(uint8 jhat, uint8 val) {
	for (Binding& it : bindings)
		if (!it.isAxis() && it.jhatAssigned() && it.getJctID() == jhat && it.getJhatVal() == val && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

void InputSys::checkBindingsA(uint8 jaxis, bool positive) {
	for (Binding& it : bindings)
		if (!it.isAxis() && ((it.jposAxisAssigned() && positive) || (it.jnegAxisAssigned() && !positive)) && it.getJctID() == jaxis && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

void InputSys::checkBindingsG(SDL_GameControllerButton gbutton) {
	for (Binding& it : bindings)
		if (!it.isAxis() && it.gbuttonAssigned() && it.getGbutton() == gbutton && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

void InputSys::checkBindingsX(SDL_GameControllerAxis gaxis, bool positive) {
	for (Binding& it : bindings)
		if (!it.isAxis() && ((it.gposAxisAssigned() && positive) || (it.gnegAxisAssigned() && !positive)) && it.getGaxis() == gaxis && it.getBcall()) {
			(World::state()->*it.getBcall())();
			break;
		}
}

bool InputSys::isPressed(Binding::Type type, float& amt) const {
	sizt i = static_cast<sizt>(type);
	return bindings[i].isAxis() ? isPressed(bindings[i], amt) : false;
}

bool InputSys::isPressed(const Binding& abind, float& amt) const {
	if (abind.keyAssigned() && SDL_GetKeyboardState(nullptr)[abind.getKey()])	// check keyboard keys
		return true;

	if (abind.jbuttonAssigned() && isPressedB(abind.getJctID()))	// check controller buttons
		return true;
	if (abind.jhatAssigned() && isPressedH(abind.getJctID(), abind.getJhatVal()))
		return true;
	if (abind.jaxisAssigned()) {	// check controller axes
		int val = getAxisJ(abind.getJctID());
		if ((abind.jposAxisAssigned() && val > 0) || (abind.jnegAxisAssigned() && val < 0)) {
			amt = axisToFloat(abind.jposAxisAssigned() ? val : -val);
			return true;
		}
	}

	if (abind.gbuttonAssigned() && isPressedG(abind.getGbutton()))	// check controller buttons
		return true;
	if (abind.gaxisAssigned()) {	// check controller axes
		int val = getAxisG(abind.getGaxis());
		if ((abind.gposAxisAssigned() && val > 0) || (abind.gnegAxisAssigned() && val < 0)) {
			amt = axisToFloat(abind.gposAxisAssigned() ? val : -val);
			return true;
		}
	}
	return false;
}

bool InputSys::isPressedB(uint8 jbutton) const {
	for (const Controller& it : controllers)
		if (!it.gamepad && SDL_JoystickGetButton(it.joystick, jbutton))
			return true;
	return false;
}

bool InputSys::isPressedG(SDL_GameControllerButton gbutton) const {
	for (const Controller& it : controllers)
		if (it.gamepad && SDL_GameControllerGetButton(it.gamepad, gbutton))
			return true;
	return false;
}

bool InputSys::isPressedH(uint8 jhat, uint8 val) const {
	for (const Controller& it : controllers)
		if (!it.gamepad)
			for (int i=0; i!=SDL_JoystickNumHats(it.joystick); i++)
				if (jhat == i && SDL_JoystickGetHat(it.joystick, i) == val)
					return true;
	return false;
}

int InputSys::getAxisJ(uint8 jaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (!it.gamepad) {
			int val = checkAxisValue(SDL_JoystickGetAxis(it.joystick, jaxis));
			if (val != 0)
				return val;
		}
	return 0;
}

int InputSys::getAxisG(SDL_GameControllerAxis gaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (it.gamepad) {
			int val = checkAxisValue(SDL_GameControllerGetAxis(it.gamepad, gaxis));
			if (val != 0)
				return val;
		}
	return 0;
}

bool InputSys::isPressedM(uint8 mbutton) {
	return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(mbutton);
}

vec2i InputSys::mousePos() {
	vec2i pos;
	SDL_GetMouseState(&pos.x, &pos.y);
	return pos;
}

void InputSys::addController(int id) {
	Controller ctr(id);
	if (ctr.index >= 0)
		controllers.push_back(ctr);
}

void InputSys::removeController(int id) {
	for (vector<Controller>::iterator it=controllers.begin(); it!=controllers.end(); it++)
		if (it->index == id) {
			it->close();
			controllers.erase(it);
			break;
		}
}

int InputSys::checkAxisValue(int value) const {
	return (std::abs(value) > World::winSys()->sets.getDeadzone()) ? value : 0;
}
