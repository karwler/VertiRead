#include "world.h"

InputSys::InputSys(const ControlsSettings& SETS) :
	sets(SETS)
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

void InputSys::MouseButtonEvent(const SDL_MouseButtonEvent& button) {
	if (captured && button.type == SDL_MOUSEBUTTONDOWN) {		// mouse button cancels keyboard capture
		if (LineEdit* box = dynamic_cast<LineEdit*>(captured))	// confirm entered text if necessary
			box->Confirm();
		SetCapture(nullptr);
	}
	
	if (button.clicks == 1) {
		if (button.button == SDL_BUTTON_LEFT) {
			if (button.type == SDL_MOUSEBUTTONDOWN)	// single left click 
				World::scene()->OnMouseDown(EClick::left);
			else
				World::scene()->OnMouseUp();		// left up
		}
		else if (button.button == SDL_BUTTON_RIGHT && button.type == SDL_MOUSEBUTTONDOWN)	// songle right click
			World::scene()->OnMouseDown(EClick::right);
	}
	else if (button.button == SDL_BUTTON_LEFT && button.type == SDL_MOUSEBUTTONDOWN)		// double left click
		World::scene()->OnMouseDown(EClick::left_double);
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(wheel.y * int(sets.scrollSpeed.y) /2);
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->Editor()->Add(text.text);
}

void InputSys::CheckShortcuts(const SDL_KeyboardEvent& key) {
	// find first shortcut with this key assigned to it
	for (const pair<string, Shortcut>& sc : sets.shortcuts)
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

bool InputSys::isPressed(byte button) {
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

Capturer* InputSys::CapturedObject() const {
	return captured;
}

void InputSys::SetCapture(Capturer* cbox) {
	captured = cbox;
	if (dynamic_cast<LineEdit*>(captured))
		SDL_StartTextInput();
	else
		SDL_StopTextInput();

	World::engine->SetRedrawNeeded();
}
