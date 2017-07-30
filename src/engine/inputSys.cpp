#include "world.h"

InputSys::InputSys(const ControlsSettings& SETS) :
	sets(SETS),
	captured(nullptr)
{
	SDL_GetMouseState(&mPos.x, &mPos.y);
	UpdateControllers();
	SetCapture(nullptr);
}

InputSys::~InputSys() {
	ClearControllers();
}

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	// different behaviour when capturing or not
	if (captured)
		captured->OnKeypress(key.keysym.scancode);
	else if (!key.repeat)	// handle only once pressed keys
		CheckShortcutsK(key.keysym.scancode);
}

void InputSys::JoystickButtonEvent(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a gamecontroller event
		return;

	if (captured)
		captured->OnJButton(jbutton.button);
	else
		CheckShortcutsB(jbutton.button);
}

void InputSys::JoystickHatEvent(const SDL_JoyHatEvent& jhat) {
	if (jhat.value == SDL_HAT_CENTERED || SDL_GameControllerFromInstanceID(jhat.which))
		return;

	if (captured)
		captured->OnJHat(jhat.hat, jhat.value);
	else
		CheckShortcutsH(jhat.hat, jhat.value);
}

void InputSys::JoystickAxisEvent(const SDL_JoyAxisEvent& jaxis) {
	int16 value = CheckAxisValue(jaxis.value);
	if (value == 0 || SDL_GameControllerFromInstanceID(jaxis.which))
		return;
	
	if (captured)
		captured->OnJAxis(jaxis.axis, (value > 0));
	else
		CheckShortcutsA(jaxis.axis, (value > 0));
}

void InputSys::GamepadButtonEvent(const SDL_ControllerButtonEvent& gbutton) {
	if (captured)
		captured->OnGButton(gbutton.button);
	else
		CheckShortcutsG(gbutton.button);
}

void InputSys::GamepadAxisEvent(const SDL_ControllerAxisEvent& gaxis) {
	int16 value = CheckAxisValue(gaxis.value);
	if (value == 0)
		return;

	if (captured)
		captured->OnGAxis(gaxis.axis, (value > 0));
	else
		CheckShortcutsX(gaxis.axis, (value > 0));
}

void InputSys::MouseMotionEvent(const SDL_MouseMotionEvent& motion) {
	mPos = vec2i(motion.x, motion.y);
	World::scene()->OnMouseMove(mPos, vec2i(motion.xrel, motion.yrel));
}

void InputSys::MouseButtonDownEvent(const SDL_MouseButtonEvent& button) {
	if (captured && !World::scene()->getPopup()) {				// mouse button cancels keyboard capture if not in popup
		if (LineEdit* box = dynamic_cast<LineEdit*>(captured))		// confirm entered text if necessary
			box->Confirm();
		SetCapture(nullptr);
	}

	if (button.clicks == 1) {
		if (button.button == SDL_BUTTON_LEFT)		// single left click
			World::scene()->OnMouseDown(mPos, EClick::left);
		else if (button.button == SDL_BUTTON_RIGHT)	// single right click
			World::scene()->OnMouseDown(mPos, EClick::right);
	} else if (button.button == SDL_BUTTON_LEFT)		// double left click
		World::scene()->OnMouseDown(mPos, EClick::left_double);
}

void InputSys::MouseButtonUpEvent(const SDL_MouseButtonEvent& button) {
	if (button.clicks == 1 && (button.button == SDL_BUTTON_LEFT || button.button == SDL_BUTTON_RIGHT))
		World::scene()->OnMouseUp();	// left/right up
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(vec2i(float(wheel.x) * sets.scrollSpeed.x, float(wheel.y) * sets.scrollSpeed.y));
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->OnText(text.text);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	World::winSys()->SetRedrawNeeded();
}

void InputSys::CheckAxisShortcuts() {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutAxis* sc = dynamic_cast<ShortcutAxis*>(it.second)) {
			float amt = 1.f;
			if (isPressed(sc, &amt) && sc->call)
				(World::program()->*sc->call)(amt);
		}
}

void InputSys::CheckShortcutsK(SDL_Scancode key) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)		// find first shortcut with this key assigned to it
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->KeyAssigned() && sc->Key() == key && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsB(uint8 jbutton) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->JButtonAssigned() && sc->JctID() == jbutton && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsH(uint8 jhat, uint8 val) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->JHatAssigned() && sc->JctID() == jhat && sc->JHatVal() == val && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsA(uint8 jaxis, bool positive) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (((sc->JPosAxisAssigned() && positive) || (sc->JNegAxisAssigned() && !positive)) && sc->JctID() == jaxis && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsG(uint8 gbutton) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->GButtonAssigned() && sc->GctID() == gbutton && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsX(uint8 gaxis, bool positive) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (((sc->GPosAxisAssigned() && positive) || (sc->GNegAxisAssigned() && !positive)) && sc->GctID() == gaxis && sc->call) {
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
	if (sc->KeyAssigned()) {			// check keyboard keys
		if (SDL_GetKeyboardState(nullptr)[sc->Key()])
			return true;
	}

	if (sc->JButtonAssigned()) {		// check controller buttons
		if (isPressedB(sc->JctID()))
			return true;
	} else if (sc->JHatAssigned()) {
		if (isPressedH(sc->JctID(), sc->JHatVal()))
			return true;
	} else if (sc->JAxisAssigned()) {		// check controller axes
		float val = getAxisJ(sc->JctID());
		if ((sc->JPosAxisAssigned() && val > 0.f) || (sc->JNegAxisAssigned() && val < 0.f)) {
			if (amt)
				*amt = (sc->JPosAxisAssigned()) ? val : -val;
			return true;
		}
	}

	if (sc->GButtonAssigned()) {		// check controller buttons
		if (isPressedG(sc->GctID()))
			return true;
	} else if (sc->GAxisAssigned()) {		// check controller axes
		float val = getAxisG(sc->GctID());
		if ((sc->GPosAxisAssigned() && val > 0.f) || (sc->GNegAxisAssigned() && val < 0.f)) {
			if (amt)
				*amt = (sc->GPosAxisAssigned()) ? val : -val;
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
	for (const Controller& it : controllers)			// get first axis that isn't 0
		if (!it.gamepad) {
			int16 val = CheckAxisValue(SDL_JoystickGetAxis(it.joystick, jaxis));
			if (val != 0)
				return axisToFloat(val);
		}
	return 0.f;
}

float InputSys::getAxisG(uint8 gaxis) const {
	for (const Controller& it : controllers)			// get first axis that isn't 0
		if (it.gamepad) {
			int16 val = CheckAxisValue(SDL_GameControllerGetAxis(it.gamepad, static_cast<SDL_GameControllerAxis>(gaxis)));
			if (val != 0)
				return axisToFloat(val);
		}
	return 0.f;
}

vec2i InputSys::mousePos() const {
	return mPos;
}

const ControlsSettings& InputSys::Settings() const {
	return sets;
}

void InputSys::ScrollSpeed(const vec2f& sspeed) {
	sets.scrollSpeed = sspeed;
}

void InputSys::Deadzone(int16 deadz) {
	sets.deadzone = deadz;
}

Shortcut* InputSys::GetShortcut(const string& name) {
	return (sets.shortcuts.count(name) == 0) ? nullptr : sets.shortcuts[name];
}

bool InputSys::HasControllers() const {
	return !controllers.empty();
}

void InputSys::UpdateControllers() {
	ClearControllers();
	for (int i=0; i!=SDL_NumJoysticks(); i++) {
		Controller it;
		if (it.Open(i))
			controllers.push_back(it);
	}
}

void InputSys::ClearControllers() {
	for (Controller& it : controllers)
		it.Close();
	controllers.clear();
}

const Capturer* InputSys::Captured() const {
	return captured;
}

void InputSys::SetCapture(Capturer* cbox) {
	captured = cbox;
	if (dynamic_cast<LineEdit*>(captured))
		SDL_StartTextInput();
	else
		SDL_StopTextInput();

	World::winSys()->SetRedrawNeeded();
}

int16 InputSys::CheckAxisValue(int16 value) const {
	return (std::abs(value) > sets.deadzone) ? value : 0;
}
