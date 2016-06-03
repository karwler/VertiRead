#pragma once

#include "items.h"

class Capturer : public ListItem {
public:
	Capturer(ListBox* SA, const string& LBL="");
	virtual ~Capturer();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key);
	ListBox* Parent() const;
};

class LineEdit : public Capturer {
public:
	LineEdit(ListBox* SA, const string& LBL="", const string& TXT="", void (Program::*KCALL)(TextEdit*)=nullptr, void (Program::*CCALL)()=nullptr, ETextType TYPE=ETextType::text);
	virtual ~LineEdit();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode key);

	TextEdit* Editor();

private:
	int textPos;
	TextEdit editor;
	void (Program::*okCall)(TextEdit*);
	void (Program::*cancelCall)();
};

class KeyGetter : public Capturer {
public:
	KeyGetter(ListBox* SA, const string& LBL="", SDL_Scancode KEY=SDL_SCANCODE_ESCAPE, void (Program::*CALLB)(SDL_Scancode)=nullptr);
	virtual ~KeyGetter();

	virtual void OnClick();
	virtual void OnKeypress(SDL_Scancode KEY);
	string KeyName() const;

private:
	SDL_Scancode key;
	void (Program::*callback)(SDL_Scancode);
};
