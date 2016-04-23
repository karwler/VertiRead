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
	if (button.button == SDL_BUTTON_LEFT) {		// so far only left ouse button is neeeded
		if (button.type == SDL_MOUSEBUTTONDOWN)
			World::scene()->OnMouseDown();
		else
			World::scene()->OnMouseUp();
	}
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	World::scene()->OnMouseWheel(wheel.y);
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	static_cast<LineEdit*>(captured)->AddText(text.text);
}

void InputSys::CheckShortcuts(const SDL_KeyboardEvent& key) {
	// find first shortcut with this key assigned to it
	for (Shortcut& sc : sets.shortcuts)
		for (SDL_Scancode& ks : sc.keys)
			if (ks == key.keysym.scancode) {
				// call shortcut function and brek out of all loops
				(World::program()->*sc.Call())();
				return;
			}
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
