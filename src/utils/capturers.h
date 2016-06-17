#pragma once

#include "items.h"

class Capturer : public ListItem {
public:
	Capturer(ListBox* SA, const string& LBL="");
	virtual ~Capturer();

	virtual void OnClick(EClick clickType);
	virtual void OnKeypress(SDL_Scancode key);
	ListBox* Parent() const;
};

class LineEdit : public Capturer {
public:
	LineEdit(ListBox* SA, const string& LBL="", const string& TXT="", ETextType TYPE=ETextType::text, void (Program::*KCALL)(const string&)=nullptr, void (Program::*CCALL)()=nullptr);
	virtual ~LineEdit();

	virtual void OnClick(EClick clickType);
	virtual void OnKeypress(SDL_Scancode key);
	
	void Confirm();
	int TextPos() const;
	void ResetTextPos();
	TextEdit* Editor();
	virtual Text getText() const;		// warning: text doesn't have global positioning
	virtual SDL_Rect getCaret() const;	// warning: caret doesn't have global positioning

protected:
	int textPos;
	TextEdit editor;
private:
	void (Program::*okCall)(const string&);
	void (Program::*cancelCall)();

	virtual void CheckCaretRight();
	virtual void CheckCaretLeft();
};

class KeyGetter : public Capturer {
public:
	KeyGetter(ListBox* SA, const string& LBL="", SDL_Scancode* KEY=nullptr);
	virtual ~KeyGetter();

	virtual void OnKeypress(SDL_Scancode KEY);
	string KeyName() const;

private:
	SDL_Scancode* key;
};
