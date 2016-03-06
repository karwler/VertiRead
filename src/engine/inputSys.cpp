#include "eventSys.h"
#include "world.h"

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	for (Shortcut& sc : sets.shortcuts)
		for (SDL_Keysym& ks : sc.keys)
			if (ks.scancode == key.keysym.scancode)
				(World::program()->*sc.Call())();
}

void InputSys::MouseButtonEvent(const SDL_MouseButtonEvent& button) {
	World::scene()->OnMouseButton();
}

void InputSys::MouseMotionEvent(const SDL_MouseMotionEvent& motion) {
	
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {

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

void InputSys::Settings(ControlsSettings settings) {
	sets = settings;
}
