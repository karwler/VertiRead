#include "world.h"

InputSys::InputSys(const ControlsSettings& SETS) :
	sets(SETS),
	captured(nullptr)
{
	SDL_GetMouseState(&lastMousePos.x, &lastMousePos.y);
	UpdateControllers();
	SetCapture(nullptr);
}

InputSys::~InputSys() {
	ClearControllers();
}

void InputSys::Tick() {
	lastMousePos = mousePos();
}

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	// different behaviour when capturing or not
	if (captured)
		captured->OnKeypress(key.keysym.scancode);
	else if (!key.repeat)	// handle only once pressed keys
		CheckShortcutsK(key.keysym.scancode);
}

void InputSys::ControllerButtonEvent(const SDL_JoyButtonEvent& jbutton) {
	if (sets.shortcuts[SHORTCUT_OK]->JButtonAssigned() && sets.shortcuts[SHORTCUT_OK]->JCtr() == jbutton.button)
		World::scene()->OnMouseDown(EClick::left, false);
	else if (captured)
		captured->OnJButton(jbutton.button);
	else
		CheckShortcutsB(jbutton.button);
}

void InputSys::ControllerHatEvent(const SDL_JoyHatEvent& jhat) {
	if (jhat.value == SDL_HAT_CENTERED)
		return;

	if (sets.shortcuts[SHORTCUT_OK]->JHatAssigned() && sets.shortcuts[SHORTCUT_OK]->JCtr() == jhat.value)
		World::scene()->OnMouseDown(EClick::left, false);
	else if (captured)
		captured->OnJHat(jhat.value);
	else
		CheckShortcutsH(jhat.value);
}

void InputSys::ControllerAxisEvent(const SDL_JoyAxisEvent& jaxis) {
	if (jaxis.value == 0)
		return;
	
	if (( (sets.shortcuts[SHORTCUT_OK]->JPosAxisAssigned() && jaxis.value > 0) || (sets.shortcuts[SHORTCUT_OK]->JNegAxisAssigned() && jaxis.value < 0)) && sets.shortcuts[SHORTCUT_OK]->JCtr() == jaxis.axis)
		World::scene()->OnMouseDown(EClick::left, false);
	else if (captured)
		captured->OnJAxis(jaxis.axis, (jaxis.value > 0));
	else
		CheckShortcutsA(jaxis.axis, (jaxis.value > 0));
}

void InputSys::MouseButtonDownEvent(const SDL_MouseButtonEvent& button) {
	if (captured && !World::scene()->getPopup()) {	// mouse button cancels keyboard capture (except if popup is shown)
		if (LineEdit* box = dynamic_cast<LineEdit*>(captured))	// confirm entered text if necessary
			box->Confirm();
		SetCapture(nullptr);
	}

	if (button.clicks == 1) {
		if (button.button == SDL_BUTTON_LEFT)		// single left click
			World::scene()->OnMouseDown(EClick::left);
		else if (button.button == SDL_BUTTON_RIGHT)	// single right click
			World::scene()->OnMouseDown(EClick::right);
	}
	else if (button.button == SDL_BUTTON_LEFT)		// double left click
		World::scene()->OnMouseDown(EClick::left_double);
}

void InputSys::MouseButtonUpEvent(const SDL_MouseButtonEvent& button) {
	if (button.clicks == 1 && (button.button == SDL_BUTTON_LEFT || button.button == SDL_BUTTON_RIGHT))
		World::scene()->OnMouseUp();	// left/right up
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(float(wheel.y) * sets.scrollSpeed.y);
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->Editor()->Add(text.text);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	World::engine()->SetRedrawNeeded();
}

void InputSys::CheckAxisShortcuts() {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutAxis* sc = dynamic_cast<ShortcutAxis*>(it.second)) {
			float amt = 1.f;
			if (isPressed(it.first, &amt) && sc->call)
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
			if (sc->JButtonAssigned() && sc->JCtr() == jbutton && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsH(uint8 jhat) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (sc->JHatAssigned() && sc->JCtr() == jhat && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

void InputSys::CheckShortcutsA(uint8 jaxis, bool positive) {
	for (const pair<string, Shortcut*>& it : sets.shortcuts)
		if (ShortcutKey* sc = dynamic_cast<ShortcutKey*>(it.second))
			if (((sc->JPosAxisAssigned() && positive) || (sc->JNegAxisAssigned() && !positive)) && sc->JCtr() == jaxis && sc->call) {
				(World::program()->*sc->call)();
				break;
			}
}

bool InputSys::isPressed(const string& holder, float* amt) const {
	if (sets.shortcuts.count(holder) == 0)	// if no holder found
		return false;

	ShortcutAxis* sc = dynamic_cast<ShortcutAxis*>(sets.shortcuts.at(holder));
	if (!sc)	// if not an axis
		return false;

	if (sc->JButtonAssigned()) {		// check controller buttons
		for (SDL_Joystick* it : controllers)
			if (SDL_JoystickGetButton(it, sc->JCtr()))
				return true;
	}
	else if (sc->JHatAssigned()) {
		for (SDL_Joystick* it : controllers)
			for (int i=0; i!=SDL_JoystickNumHats(it); i++)
				if (SDL_JoystickGetHat(it, i) == sc->JCtr())
					return true;
	}
	else if (sc->JAxisAssigned()) {		// check controller axes
		float val = getAxis(sc->JCtr());
		if (val != 0.f && sc->JPosAxisAssigned()) {
			if (amt)
				*amt = sc->JPosAxisAssigned() ? val : -val;
			return true;
		}
	}
	if (sc->KeyAssigned())				// check keyboard keys
		return SDL_GetKeyboardState(nullptr)[sc->Key()];
	return false;
}

bool InputSys::isPressedK(SDL_Scancode key) {
	return SDL_GetKeyboardState(nullptr)[key];
}

bool InputSys::isPressedM(uint32 button) {
	return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(button);
}

bool InputSys::isPressedC(uint8 jbutton) const {
	for (SDL_Joystick* it : controllers)
		if (SDL_JoystickGetButton(it, jbutton))
			return true;
	return false;
}

float InputSys::getAxis(uint8 axis) const {
	for (SDL_Joystick* it : controllers) {			// get first axis that isn't 0
		int16 val = SDL_JoystickGetAxis(it, axis);
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

vec2i InputSys::mouseMove() const {
	return mousePos() - lastMousePos;
}

ControlsSettings InputSys::Settings() const {
	return sets;
}

void InputSys::ScrollSpeed(const vec2f& sspeed) {
	sets.scrollSpeed = sspeed;
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
		SDL_Joystick* it = SDL_JoystickOpen(i);
		if (it)
			controllers.push_back(it);
	}
}

void InputSys::ClearControllers() {
	for (SDL_Joystick* it : controllers)
		SDL_JoystickClose(it);
	controllers.clear();
}

Capturer* InputSys::Captured() const {
	return captured;
}

void InputSys::SetCapture(Capturer* cbox) {
	captured = cbox;
	if (dynamic_cast<LineEdit*>(captured))
		SDL_StartTextInput();
	else
		SDL_StopTextInput();

	World::engine()->SetRedrawNeeded();
}
