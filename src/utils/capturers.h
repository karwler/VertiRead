#pragma once

#include "items.h"

class Capturer : public ListItem {
public:
	Capturer(ScrollAreaX1* SA, const string& LBL="");
	virtual ~Capturer();

	virtual void OnClick(ClickType click);
	virtual void OnKeypress(SDL_Scancode key) = 0;
	virtual void OnJButton(uint8 jbutton) = 0;
	virtual void OnJHat(uint8 jhat, uint8 value) = 0;
	virtual void OnJAxis(uint8 jaxis, bool positive) = 0;
	virtual void OnGButton(uint8 gbutton) = 0;
	virtual void OnGAxis(uint8 gaxis, bool positive) = 0;

	ScrollAreaX1* Parent() const;
};

class LineEdit : public Capturer {
public:
	LineEdit(ScrollAreaX1* SA, const string& LBL="", const string& TXT="", ETextType TYPE=ETextType::text, void (Program::*KCALL)(const string&)=nullptr, void (Program::*CCALL)()=nullptr);
	virtual ~LineEdit();

	virtual void OnClick(ClickType click);
	virtual void OnKeypress(SDL_Scancode key);
	virtual void OnJButton(uint8 jbutton);
	virtual void OnJHat(uint8 jhat, uint8 value);
	virtual void OnJAxis(uint8 jaxis, bool positive);
	virtual void OnGButton(uint8 gbutton);
	virtual void OnGAxis(uint8 gaxis, bool positive);
	void OnText(const char* text);
	
	void Confirm();
	void Cancel();
	int TextPos() const;
	void ResetTextPos();
	TextEdit& Editor();
	const TextEdit& Editor() const;
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
	enum class EAcceptType : uint8 {
		keyboard,
		joystick,
		gamepad
	};

	KeyGetter(ScrollAreaX1* SA, EAcceptType ACT, Shortcut* SHC=nullptr);
	virtual ~KeyGetter();

	virtual void OnKeypress(SDL_Scancode key);
	virtual void OnJButton(uint8 jbutton);
	virtual void OnJHat(uint8 jhat, uint8 value);
	virtual void OnJAxis(uint8 jaxis, bool positive);
	virtual void OnGButton(uint8 gbutton);
	virtual void OnGAxis(uint8 gaxis, bool positive);
	
	string Text() const;

private:
	EAcceptType acceptType;
	Shortcut* shortcut;
};
