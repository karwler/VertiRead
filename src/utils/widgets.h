#pragma once

#include "settings.h"

// size of a widget in pixels or relative to it's parent
struct Size {
	Size(int PIX);
	Size(float PRC=1.f);

	bool usePix;
	union {
		int pix;	// use if type is pix
		float prc;	// use if type is prc
	};

	void set(int PIX);
	void set(float PRC);
};
using vec2s = vec2<Size>;

// can be used as spacer
class Widget {
public:
	Widget(const Size& SIZ=Size());	// parent and id should be set in Layout's setWidgets
	virtual ~Widget() {}

	virtual void drawSelf() {}	// calls appropriate drawing function(s) in DrawSys
	virtual void onResize() {}	// for updating values when window size changed
	virtual void tick(float dSec) {}
	virtual void postInit() {}	// gets called after parent is set and all set up
	virtual void onClick(const vec2i& mPos, uint8 mBut) {}
	virtual void onDoubleClick(const vec2i& mPos, uint8 mBut) {}
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov) {}
	virtual void onHold(const vec2i& mPos, uint8 mBut) {}
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov) {}	// mouse move while left button down
	virtual void onUndrag(uint8 mBut) {}	// get's called on mouse button up if instance is Scene's capture
	virtual void onScroll(const vec2i& wMov) {}	// on mouse wheel y movement
	virtual void onKeypress(const SDL_Keysym& key) {}
	virtual void onJButton(uint8 jbutton) {}
	virtual void onJHat(uint8 jhat, uint8 value) {}
	virtual void onJAxis(uint8 jaxis, bool positive) {}
	virtual void onGButton(SDL_GameControllerButton gbutton) {}
	virtual void onGAxis(SDL_GameControllerAxis gaxis, bool positive) {}
	virtual void onText(const string& str) {}
	virtual void onSelectUp();
	virtual void onSelectDown();
	virtual void onSelectLeft();
	virtual void onSelectRight();
	virtual bool selectable() const { return false; }

	sizt getID() const { return pcID; }
	Layout* getParent() const { return parent; }
	void setParent(Layout* PNT, sizt ID);

	const Size& getRelSize() const { return relSize; }
	virtual vec2i position() const;
	virtual vec2i size() const;
	vec2i center() const;
	SDL_Rect rect() const;	// the rectangle that is the widget
	virtual SDL_Rect parentFrame() const;
	virtual SDL_Rect frame() const { return parentFrame(); }	// the rectangle to restrain the children's visibility (regular widgets don't have one, only scroll areas)

protected:
	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizt pcID;		// this widget's id in parent's widget list
	Size relSize;	// size relative to parent's parameters
};

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Widget {
public:
	Button(const Size& SIZ=Size(), void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr);
	virtual ~Button() {}

	virtual void drawSelf();
	virtual void onClick(const vec2i& mPos, uint8 mBut);
	virtual void onDoubleClick(const vec2i& mPos, uint8 mBut);
	virtual bool selectable() const { return true; }

protected:
	void (Program::*lcall)(Button*);
	void (Program::*rcall)(Button*);
	void (Program::*dcall)(Button*);
};

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	CheckBox(const Button& BASE=Button(), bool ON=false);
	virtual ~CheckBox() {}

	virtual void drawSelf();
	virtual void onClick(const vec2i& mPos, uint8 mBut);

	SDL_Rect boxRect() const;

	bool on;
};

// horizontal slider (maybe one day it'll be able to be vertical)
class Slider : public Button {
public:
	Slider(const Button& BASE=Button(), int VAL=0, int MIN=0, int MAX=255);
	virtual ~Slider() {}

	virtual void drawSelf();
	virtual void onClick(const vec2i& mPos, uint8 mBut);
	virtual void onHold(const vec2i& mPos, uint8 mBut);
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov);
	virtual void onUndrag(uint8 mBut);

	int getVal() const { return val; }
	void setVal(int VAL);

	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;

private:
	int val, min, max;
	int diffSliderMouse;

	void setSlider(int xpos);
	int sliderPos() const;
	int sliderLim() const;
};

// can have multiple images through which it cycles when presed
class Picture : public Button {
public:
	Picture(const Button& BASE=Button(), const string& TEX="");
	virtual ~Picture();

	virtual void drawSelf();

	const vec2i& getRes() const { return res; }
	const string& getFile() const { return file; }

	SDL_Texture* tex;
private:
	vec2i res;
	string file;
};

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right
	};

	Label(const Button& BASE=Button(), const string& TXT="", Alignment ALG=Alignment::left);
	virtual ~Label();

	virtual void drawSelf();
	virtual void postInit();

	const string& getText() const { return text; }
	virtual void setText(const string& str);
	SDL_Rect textRect() const;

	Alignment align;	// text alignment
	SDL_Texture* tex;	// rendered text
protected:
	string text;
	vec2i textSize;

	virtual vec2i textPos() const;
	void updateTex();
};

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
public:
	SwitchBox(const Button& BASE=Button(), const vector<string>& OPTS={}, const string& COP="", Alignment ALG=Alignment::left);
	virtual ~SwitchBox() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut);

private:
	vector<string> options;
	sizt curOpt;

	void shiftOption(int ofs);
};

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm rather than on click)
class LineEdit : public Label {
public:
	enum class TextType : uint8 {
		text,
		sInteger,
		sIntegerSpaced,
		uInteger,
		uIntegerSpaced,
		sFloating,
		sFloatingSpaced,
		uFloating,
		uFloatingSpaced
	};

	LineEdit(const Button& BASE=Button(), const string& TXT="", TextType TYP=TextType::text);
	virtual ~LineEdit() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut);
	virtual void onKeypress(const SDL_Keysym& key);
	virtual void onText(const string& str);

	const string& getOldText() const { return oldText; }
	virtual void setText(const string& str);
	SDL_Rect caretRect() const;	

	void confirm();
	void cancel();

private:
	int textOfs;		// text's horizontal offset
	sizt cpos;		// caret position
	TextType textType;
	string oldText;

	virtual vec2i textPos() const;
	int caretPos() const;	// caret's relative x position
	void setCPos(sizt cp);

	sizt findWordStart();	// returns index of first character of word before cpos
	sizt findWordEnd();		// returns index of character after last character of word after cpos
	void cleanText();
	void cleanSIntSpacedText(sizt i=0);
	void cleanUIntText(sizt i=0);
	void cleanUIntSpacedText();
	void cleanSFloatSpacedText(sizt i=0);
	void cleanUFloatText(sizt i=0);
	void cleanUFloatSpacedText();
};

// for getting a key/button/axis
class KeyGetter : public Label {
public:
	enum class AcceptType : uint8 {
		keyboard,
		joystick,
		gamepad
	};

	KeyGetter(const Button& BASE=Button(), AcceptType ACT=AcceptType::keyboard, Binding::Type BND=Binding::Type::numBindings);
	virtual ~KeyGetter() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut);
	virtual void onKeypress(const SDL_Keysym& key);
	virtual void onJButton(uint8 jbutton);
	virtual void onJHat(uint8 jhat, uint8 value);
	virtual void onJAxis(uint8 jaxis, bool positive);
	virtual void onGButton(SDL_GameControllerButton gbutton);
	virtual void onGAxis(SDL_GameControllerAxis gaxis, bool positive);

private:
	AcceptType acceptType;		// what kind of binding is being accepted
	Binding::Type bindingType;	// index of what is currently being edited
};
