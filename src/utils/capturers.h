#pragma once

#include "objects.h"

enum class ETextType : byte {
	text,
	integer,
	floating
};

class Capturer : public Object {
public:
	Capturer(const Object& BASE=Object());
	virtual ~Capturer();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key) = 0;
};

class LineEdit : public Capturer {
public:
	LineEdit(const Object& BASE=Object(), string LBL="", string TXT="", void (Program::*KCALL)(TextEdit*)=nullptr, void (Program::*CCALL)()=nullptr, ETextType TYPE=ETextType::text);
	virtual ~LineEdit();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key);
	void AddText(string text);
	Text getText(vec2i* sideCrop=nullptr) const;
	Text getLabel() const;
	TextEdit* Editor() const;

private:
	string label;
	ETextType type;
	TextEdit editor;
	int textPos;
	void (Program::*okCall)(TextEdit*);
	void (Program::*cancelCall)();

	string CheckText(string str);
};

class KeyGetter : public Capturer {
public:
	KeyGetter(const Object& BASE=Object(), string LBL="", SDL_Scancode KEY=SDL_SCANCODE_ESCAPE, void (Program::*CALLB)(SDL_Scancode)=nullptr);
	virtual ~KeyGetter();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode KEY);
	Text getText() const;
	Text getLabel() const;

private:
	string label;
	SDL_Scancode key;
	void (Program::*callback)(SDL_Scancode);
};
