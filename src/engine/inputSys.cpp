#include "inputSys.h"
#include "world.h"

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	if (key.type == SDL_KEYDOWN) {
		// find first shortcut with this key assigned to it
		for (Shortcut& sc : sets.shortcuts)
			for (SDL_Keysym& ks : sc.keys)
				if (ks.scancode == key.keysym.scancode) {
					// call shortcut function and brek out of all loops
					(World::program()->*sc.Call())();
					return;
				}
	}
}

void InputSys::MouseButtonEvent(const SDL_MouseButtonEvent& button) {
	if (button.button == SDL_BUTTON_LEFT) {		// so far only left ouse button is neeeded
		if (button.type == SDL_MOUSEBUTTONDOWN)
			World::scene()->OnMouseDown();
		else
			World::scene()->OnMouseUp();
	}
}

void InputSys::MouseMotionEvent(const SDL_MouseMotionEvent& motion) {
	if (motion.state == SDL_PRESSED)	// process further if left mouse button is pressed
		World::scene()->OnMouseDrag();
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
