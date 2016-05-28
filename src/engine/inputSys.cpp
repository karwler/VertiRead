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
	if (dynamic_cast<KeyGetter*>(captured))			// mouse button cancels key capture
		World::program()->Event_Back();
	else if (button.clicks == 1) {
		if (button.button == SDL_BUTTON_LEFT) {		// so far only left mouse button is neeeded
			if (button.type == SDL_MOUSEBUTTONDOWN)
				World::scene()->OnMouseDown(false);
			else
				World::scene()->OnMouseUp();
		}
	}
	else if (button.type == SDL_MOUSEBUTTONDOWN && button.button == SDL_BUTTON_LEFT)	// double left click
		World::scene()->OnMouseDown(true);
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(wheel.y * int(sets.scrollSpeed.y) /2);
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->AddText(text.text);
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

ControlsSettings InputSys::Settings() const {
	return sets;
}

void InputSys::Settings(const ControlsSettings& settings) {
	sets = settings;
}

vec2i InputSys::mouseMove() const {
	return mousePos() - lastMousePos;
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
}
