#include "world.h"

InputSys::InputSys(const ControlsSettings& SETS) :
	sets(SETS),
	captured(nullptr)
{
	SDL_GetMouseState(&lastMousePos.x, &lastMousePos.y);
	SetCapture(nullptr);
}

void InputSys::Tick() {
	lastMousePos = mousePos();
}

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	// different behaviour when capturing or not
	if (captured)
		captured->OnKeypress(key.keysym.scancode);
	else if (!key.repeat)	// handle only once pressed keys
		CheckShortcuts(key);
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
	if (button.clicks == 1 && button.button == SDL_BUTTON_LEFT)
		World::scene()->OnMouseUp();	// left up
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(wheel.y * int(sets.scrollSpeed.y) /2);
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->Editor()->Add(text.text);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	World::engine()->SetRedrawNeeded();
}

void InputSys::CheckShortcuts(const SDL_KeyboardEvent& key) {
	for (const pair<string, Shortcut>& sc : sets.shortcuts)		// find first shortcut with this key assigned to it
		if (sc.second.key == key.keysym.scancode) {
			(World::program()->*sc.second.call)();
			break;
		}
}

bool InputSys::isPressed(const string& key) const {
	return SDL_GetKeyboardState(nullptr)[sets.holders.at(key)];
}

bool InputSys::isPressed(SDL_Scancode key) {
	return SDL_GetKeyboardState(nullptr)[key];
}

bool InputSys::isPressed(uint8 button) {
	return SDL_GetMouseState(nullptr, nullptr) & button;
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

SDL_Scancode* InputSys::GetKeyPtr(const string& name, bool shortcut) {
	return shortcut ? &sets.shortcuts[name].key : &sets.holders[name];
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
