#pragma once

#include "items.h"

// abstract class for an item that can hog all input to itself
class Capturer : public ListItem {
public:
	Capturer(ScrollAreaItems* SA, const string& LBL="");
	virtual ~Capturer();

	virtual void onClick(ClickType click);
	virtual void onKeypress(SDL_Scancode key) = 0;
	virtual void onJButton(uint8 jbutton) = 0;
	virtual void onJHat(uint8 jhat, uint8 value) = 0;
	virtual void onJAxis(uint8 jaxis, bool positive) = 0;
	virtual void onGButton(uint8 gbutton) = 0;
	virtual void onGAxis(uint8 gaxis, bool positive) = 0;
};

// for editing text
class LineEdit : public Capturer {
public:
	LineEdit(ScrollAreaItems* SA, const string& LBL="", const string& TXT="", ETextType TYPE=ETextType::text, void (Program::*KCALL)(const string&)=nullptr, void (Program::*CCALL)()=nullptr);
	virtual ~LineEdit();

	virtual void onClick(ClickType click);
	virtual void onKeypress(SDL_Scancode key);
	virtual void onJButton(uint8 jbutton);
	virtual void onJHat(uint8 jhat, uint8 value);
	virtual void onJAxis(uint8 jaxis, bool positive);
	virtual void onGButton(uint8 gbutton);
	virtual void onGAxis(uint8 gaxis, bool positive);
	void onText(const char* text);
	
	void confirm();
	void cancel();
	int getTextPos() const;
	void resetTextPos();
	TextEdit& getEditor();
	virtual Text text() const;		// warning: text doesn't have global positioning
	virtual SDL_Rect caretRect() const;	// warning: caret doesn't have global positioning

protected:
	int textPos;		// text's horizontal offset
	TextEdit editor;	// handles the actual text editing

private:
	void (Program::*okCall)(const string&);
	void (Program::*cancelCall)();

	virtual void checkCaretRight();
	virtual void checkCaretLeft();
};

// for getting a key/button/axis
class KeyGetter : public Capturer {
public:
	enum class EAcceptType : uint8 {
		keyboard,
		joystick,
		gamepad
	};

	KeyGetter(ScrollAreaItems* SA, EAcceptType ACT, Shortcut* SHC=nullptr);
	virtual ~KeyGetter();

	virtual void onKeypress(SDL_Scancode key);
	virtual void onJButton(uint8 jbutton);
	virtual void onJHat(uint8 jhat, uint8 value);
	virtual void onJAxis(uint8 jaxis, bool positive);
	virtual void onGButton(uint8 gbutton);
	virtual void onGAxis(uint8 gaxis, bool positive);
	
	string text() const;

private:
	EAcceptType acceptType;	// what kind of shortcut is being accepted
	Shortcut* shortcut;		// pointer to the shortcut that is currently being edited
};
