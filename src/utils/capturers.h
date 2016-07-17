#pragma once

#include "items.h"

class Capturer : public ListItem {
public:
	Capturer(ListBox* SA, const string& LBL="");
	virtual ~Capturer();

	virtual void OnClick(EClick clickType);
	virtual void OnKeypress(SDL_Scancode key) = 0;
	virtual void OnJButton(uint8 jbutton) = 0;
	virtual void OnJHat(uint8 jhat) = 0;
	virtual void OnJAxis(uint8 jaxis, bool positive) = 0;
	ListBox* Parent() const;
};

class LineEdit : public Capturer {
public:
	LineEdit(ListBox* SA, const string& LBL="", const string& TXT="", ETextType TYPE=ETextType::text, void (Program::*KCALL)(const string&)=nullptr, void (Program::*CCALL)()=nullptr);
	virtual ~LineEdit();

	virtual void OnClick(EClick clickType);
	virtual void OnKeypress(SDL_Scancode key);
	virtual void OnJButton(uint8 jbutton);
	virtual void OnJHat(uint8 jhat);
	virtual void OnJAxis(uint8 jaxis, bool positive);
	
	void Confirm();
	void Cancel();
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
	KeyGetter(ListBox* SA, const string& LBL="", Shortcut* SHC=nullptr);
	virtual ~KeyGetter();

	virtual void OnKeypress(SDL_Scancode key);
	virtual void OnJButton(uint8 jbutton);
	virtual void OnJHat(uint8 jhat);
	virtual void OnJAxis(uint8 jaxis, bool positive);
	
	string Text() const;

private:
	Shortcut* shortcut;
};
