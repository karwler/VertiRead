#pragma once

#include "utils/types.h"

enum class ECapture : uint8 {
	NONE,
	CP,
	LE
};

class InputSys {
public:
	InputSys(const ControlsSettings& SETS=ControlsSettings());

	void Tick();
	void KeypressEvent(const SDL_KeyboardEvent& key);
	void MouseButtonDownEvent(const SDL_MouseButtonEvent& button);
	void MouseButtonUpEvent(const SDL_MouseButtonEvent& button);
	void MouseWheelEvent(const SDL_MouseWheelEvent& wheel);
	void TextEvent(const SDL_TextInputEvent& text);

	bool isPressed(const string& key) const;	// looks through holders in controls settings
	static bool isPressed(SDL_Scancode key);
	static bool isPressed(uint8 button);
	static vec2i mousePos();
	vec2i mouseMove() const;

	ControlsSettings Settings() const;
	void ScrollSpeed(const vec2f& sspeed);
	SDL_Scancode* GetKeyPtr(const string& name, bool shortcut);

	Capturer* CapturedCP() const;
	Capturer* CapturedLE() const;
	ECapture CurCaptured() const;
	void ResetCapture();
	void SetCaptureCP(Capturer* cbox);
	void SetCaptureLE(LineEditor* cbox);

private:
	ControlsSettings sets;
	vec2i lastMousePos;

	// this shit's needed, cause of pointer value issues
	union {
		Capturer* capCP;
		LineEditor* capLE;
	};
	ECapture curCap;

	void CheckShortcuts(const SDL_KeyboardEvent& key);
};
