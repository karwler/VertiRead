#pragma once

#include "utils/types.h"

class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());

	void Tick();
	void KeypressEvent(const SDL_KeyboardEvent& key);
	void MouseButtonEvent(const SDL_MouseButtonEvent& button);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);
	void TextEvent(const SDL_TextInputEvent& text);

	bool isPressed(string key) const;			// looks through holders in controls settings
	static bool isPressed(SDL_Scancode key);
	static bool isPressed(byte button);
	static vec2i mousePos();

	ControlsSettings Settings() const;
	void Settings(const ControlsSettings& settings);
	vec2i mouseMove() const;
	Capturer* CapturedObject() const;
	void SetCapture(Capturer* cbox);

private:
	ControlsSettings sets;
	vec2i lastMousePos;
	Capturer* captured;

	void CheckShortcuts(const SDL_KeyboardEvent& key);
};
