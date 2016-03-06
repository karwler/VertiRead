#pragma once

#include "utils/types.h"

class InputSys {
public:
	void KeypressEvent(const SDL_KeyboardEvent& key);
	void MouseButtonEvent(const SDL_MouseButtonEvent& button);
	void MouseMotionEvent(const SDL_MouseMotionEvent& motion);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);

	static bool isPressed(SDL_Scancode key);
	static bool isPressed(byte button);
	static vec2i mousePos();

	ControlsSettings Settings() const;
	void Settings(ControlsSettings settings);

private:
	ControlsSettings sets;
};
