#pragma once

#include "utils/types.h"

class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());

	void Tick();
	void KeypressEvent(const SDL_KeyboardEvent& key);
	void MouseButtonEvent(const SDL_MouseButtonEvent& button);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);

	static bool isPressed(SDL_Scancode key);
	static bool isPressed(byte button);
	static vec2i mousePos();

	ControlsSettings Settings() const;
	void Settings(const ControlsSettings& settings);
	vec2i mouseMove() const;

private:
	ControlsSettings sets;
	vec2i lastMousePos;
};
