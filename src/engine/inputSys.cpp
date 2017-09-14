#include "world.h"

InputSys::InputSys(const ControlsSettings& SETS) :
	sets(SETS)
{
	updateControllers();
	setCaptured(nullptr);
}

InputSys::~InputSys() {
	clearControllers();
}

void InputSys::eventKeypress(const SDL_KeyboardEvent& key) {
	// different behaviour when capturing or not
	if (captured)
		captured->onKeypress(key.keysym.scancode);
	else if (!key.repeat)	// handle only once pressed keys
		checkShortcutsK(key.keysym.scancode);
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a gamecontroller event
		return;
	
	if (captured)
		captured->onJButton(jbutton.button);
	else
		checkShortcutsB(jbutton.button);
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) {
	if (jhat.value == SDL_HAT_CENTERED || SDL_GameControllerFromInstanceID(jhat.which))
		return;

	if (captured)
		captured->onJHat(jhat.hat, jhat.value);
	else
		checkShortcutsH(jhat.hat, jhat.value);
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) {
	int16 value = checkAxisValue(jaxis.value);
	if (value == 0 || SDL_GameControllerFromInstanceID(jaxis.which))
		return;
	
	if (captured)
		captured->onJAxis(jaxis.axis, (value > 0));
	else
		checkShortcutsA(jaxis.axis, (value > 0));
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	if (captured)
		captured->onGButton(gbutton.button);
	else
		checkShortcutsG(gbutton.button);
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	int16 value = checkAxisValue(gaxis.value);
	if (value == 0)
		return;

	if (captured)
		captured->onGAxis(gaxis.axis, (value > 0));
	else
		checkShortcutsX(gaxis.axis, (value > 0));
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion) {
	mouseMove = vec2i(motion.xrel, motion.yrel);
	World::scene()->onMouseMove(vec2i(motion.x, motion.y), mouseMove);
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button) {
	if (captured && !World::scene()->getPopup() && button.button == SDL_BUTTON_LEFT) {	// left mouse button cancels keyboard capture if not in popup
		if (LineEdit* box = dynamic_cast<LineEdit*>(captured))		// confirm entered text if such a thing exists
			box->confirm();
		setCaptured(nullptr);
	}
	World::scene()->onMouseDown(mousePos(), ClickType(button.button, button.clicks));
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button) {
	World::scene()->onMouseUp(vec2i(button.x, button.y), ClickType(button.button, button.clicks));
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	World::scene()->onMouseWheel(vec2i(float(wheel.x) * sets.scrollSpeed.x, float(wheel.y) * sets.scrollSpeed.y));
}

void InputSys::eventText(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->onText(text.text);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	World::winSys()->setRedrawNeeded();
}

void InputSys::tick(float dSec) {
	// handle keyhold
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutAxis* sc = dynamic_cast<ShortcutAxis*>(it.second)) {
			float amt = 1.f;
			if (isPressed(sc, &amt) && sc->call)
				(World::program()->*sc->call)(amt);
		}
}

void InputSys::checkShortcutsK(SDL_Scancode key) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)	// find first shortcut with this key assigned to it
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->keyAssigned() && sc->getKey() == key && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::checkShortcutsB(uint8 jbutton) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->jbuttonAssigned() && sc->getJctID() == jbutton && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::checkShortcutsH(uint8 jhat, uint8 val) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->jhatAssigned() && sc->getJctID() == jhat && sc->getJhatVal() == val && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::checkShortcutsA(uint8 jaxis, bool positive) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (((sc->jposAxisAssigned() && positive) || (sc->jnegAxisAssigned() && !positive)) && sc->getJctID() == jaxis && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::checkShortcutsG(uint8 gbutton) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->gbuttonAssigned() && sc->getGctID() == gbutton && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::checkShortcutsX(uint8 gaxis, bool positive) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (((sc->gposAxisAssigned() && positive) || (sc->gnegAxisAssigned() && !positive)) && sc->getGctID() == gaxis && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

bool InputSys::isPressed(const string& holder, float* amt) const {
	if (sets.shortcuts.count(holder) == 0)	// if no holder found
		return false;

	if (ShortcutAxis* sc = dynamic_cast<ShortcutAxis*>(sets.shortcuts.at(holder)))
		return isPressed(sc, amt);
	return false;
}

bool InputSys::isPressed(const ShortcutAxis* sc, float* amt) const {
	if (sc->keyAssigned()) {	// check keyboard keys
		if (SDL_GetKeyboardState(nullptr)[sc->getKey()])
			return true;
	}

	if (sc->jbuttonAssigned()) {	// check controller buttons
		if (isPressedB(sc->getJctID()))
			return true;
	} else if (sc->jhatAssigned()) {
		if (isPressedH(sc->getJctID(), sc->getJhatVal()))
			return true;
	} else if (sc->jaxisAssigned()) {	// check controller axes
		float val = getAxisJ(sc->getJctID());
		if ((sc->jposAxisAssigned() && val > 0.f) || (sc->jnegAxisAssigned() && val < 0.f)) {
			if (amt)
				*amt = (sc->jposAxisAssigned()) ? val : -val;
			return true;
		}
	}

	if (sc->gbuttonAssigned()) {	// check controller buttons
		if (isPressedG(sc->getGctID()))
			return true;
	} else if (sc->gaxisAssigned()) {	// check controller axes
		float val = getAxisG(sc->getGctID());
		if ((sc->gposAxisAssigned() && val > 0.f) || (sc->gnegAxisAssigned() && val < 0.f)) {
			if (amt)
				*amt = (sc->gposAxisAssigned()) ? val : -val;
			return true;
		}
	}
	return false;
}

bool InputSys::isPressedK(SDL_Scancode key) {
	return SDL_GetKeyboardState(nullptr)[key];
}

bool InputSys::isPressedM(uint32 button) {
	return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(button);
}

bool InputSys::isPressedB(uint8 jbutton) const {
	for (const Controller& it : controllers)
		if (!it.gamepad && SDL_JoystickGetButton(it.joystick, jbutton))
			return true;
	return false;
}

bool InputSys::isPressedG(uint8 gbutton) const {
	for (const Controller& it : controllers)
		if (it.gamepad && SDL_GameControllerGetButton(it.gamepad, static_cast<SDL_GameControllerButton>(gbutton)))
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

float InputSys::getAxisJ(uint8 jaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (!it.gamepad) {
			int16 val = checkAxisValue(SDL_JoystickGetAxis(it.joystick, jaxis));
			if (val != 0)
				return axisToFloat(val);
		}
	return 0.f;
}

float InputSys::getAxisG(uint8 gaxis) const {
	for (const Controller& it : controllers)	// get first axis that isn't 0
		if (it.gamepad) {
			int16 val = checkAxisValue(SDL_GameControllerGetAxis(it.gamepad, static_cast<SDL_GameControllerAxis>(gaxis)));
			if (val != 0)
				return axisToFloat(val);
		}
	return 0.f;
}

vec2i InputSys::mousePos() {
	vec2i pos;
	SDL_GetMouseState(&pos.x, &pos.y);
	return pos;
}

vec2i InputSys::getMouseMove() const {
	return mouseMove;
}

const ControlsSettings& InputSys::getSettings() const {
	return sets;
}

void InputSys::setScrollSpeed(const vec2f& sspeed) {
	sets.scrollSpeed = sspeed;
}

void InputSys::setDeadzone(int16 deadz) {
	sets.deadzone = deadz;
}

Shortcut* InputSys::getShortcut(const string& name) {
	return (sets.shortcuts.count(name) == 0) ? nullptr : sets.shortcuts[name];
}

bool InputSys::hasControllers() const {
	return !controllers.empty();
}

void InputSys::updateControllers() {
	clearControllers();
	for (int i=0; i!=SDL_NumJoysticks(); i++) {
		Controller it;
		if (it.open(i))
			controllers.push_back(it);
	}
}

void InputSys::clearControllers() {
	for (Controller& it : controllers)
		it.close();
	controllers.clear();
}

const Capturer* InputSys::getCaptured() const {
	return captured;
}

void InputSys::setCaptured(Capturer* cbox) {
	captured = cbox;
	if (dynamic_cast<LineEdit*>(captured))	// if it's a LineEdit, start text input capture
		SDL_StartTextInput();
	else
		SDL_StopTextInput();

	World::winSys()->setRedrawNeeded();
}

int16 InputSys::checkAxisValue(int16 value) const {
	return (std::abs(value) > sets.deadzone) ? value : 0;
}
