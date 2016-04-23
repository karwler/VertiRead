#pragma once

#include "objects.h"

class Capturer : public Object {
public:
	Capturer(const Object& BASE=Object());
	virtual ~Capturer();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key) = 0;
};

class LineEdit : public Capturer {
public:
	LineEdit(const Object& BASE=Object(), string TXT="");
	virtual ~LineEdit();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key);
	void AddText(cstr text);
	Text getText(vec2i* sideCrop=nullptr) const;
	TextEdit* Editor() const;

private:
	TextEdit editor;
	int textPos;
};

class KeyGetter : public Capturer {
public:
	KeyGetter(const Object& BASE=Object(), SDL_Scancode KEY=SDL_SCANCODE_ESCAPE);	// escape means none
	virtual ~KeyGetter();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode KEY);
	Text getText() const;

private:
	SDL_Scancode key;
};
