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
	Widget(const Size& SIZ=Size(), Layout* PNT=nullptr, sizt ID=SIZE_MAX);	// parent and id should be set by Layout
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
	virtual void onNavSelectUp();
	virtual void onNavSelectDown();
	virtual void onNavSelectLeft();
	virtual void onNavSelectRight();
	virtual bool navSelectable() const { return false; }

	sizt getID() const { return pcID; }
	Layout* getParent() const { return parent; }
	void setParent(Layout* PNT, sizt ID);

	const Size& getRelSize() const { return relSize; }
	virtual vec2i position() const;
	virtual vec2i size() const;
	vec2i center() const;
	SDL_Rect rect() const;	// the rectangle that is the widget
	virtual SDL_Rect frame() const;	// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)	

protected:
	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizt pcID;		// this widget's id in parent's widget list
	Size relSize;	// size relative to parent's parameters
};

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Widget {
public:
	Button(const Size& SIZ=Size(), void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, SDL_Texture* TEX=nullptr, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
	virtual ~Button() {}

	virtual void drawSelf();
	virtual void onClick(const vec2i& mPos, uint8 mBut);
	virtual void onDoubleClick(const vec2i& mPos, uint8 mBut);
	virtual bool navSelectable() const { return true; }
	
	Color color();
	vec2i texRes() const;
	virtual SDL_Rect texRect() const;

	SDL_Texture* tex;
	bool showBG;
	int margin;
protected:
	void (Program::*lcall)(Button*);
	void (Program::*rcall)(Button*);
	void (Program::*dcall)(Button*);
};

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	CheckBox(const Size& SIZ=Size(), bool ON=false, void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, SDL_Texture* TEX=nullptr, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
	virtual ~CheckBox() {}

	virtual void drawSelf();
	virtual void onClick(const vec2i& mPos, uint8 mBut);

	SDL_Rect boxRect() const;
	Color boxColor() const;

	bool on;
};

// horizontal slider (maybe one day it'll be able to be vertical)
class Slider : public Button {
public:
	Slider(const Size& SIZ=Size(), int VAL=0, int MIN=0, int MAX=255, void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, SDL_Texture* TEX=nullptr, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
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

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right
	};
	Label(const Size& SIZ=Size(), const string& TXT="", void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, Alignment ALG=Alignment::left, SDL_Texture* TEX=nullptr, int TMG=Default::textMargin, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
	virtual ~Label();

	virtual void drawSelf();
	virtual void postInit();

	const string& getText() const { return text; }
	virtual void setText(const string& str);
	SDL_Rect textRect() const;
	SDL_Rect textFrame() const;
	virtual SDL_Rect texRect() const;
	int textIconOffset() const;

	Alignment align;	// text alignment
	SDL_Texture* textTex;
protected:
	string text;
	int textMargin;

	virtual vec2i textPos() const;
	vec2i textSize() const;
	void updateTex();
};

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
public:
	SwitchBox(const Size& SIZ=Size(), const vector<string>& OPTS={}, const string& COP="", void (Program::*CCL)(Button*)=nullptr, Alignment ALG=Alignment::left, SDL_Texture* TEX=nullptr, int TMG=Default::textMargin, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
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
	LineEdit(const Size& SIZ=Size(), const string& TXT="", void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, TextType TYP=TextType::text, SDL_Texture* TEX=nullptr, int TMG=Default::textMargin, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
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
	KeyGetter(const Size& SIZ=Size(), AcceptType ACT=AcceptType::keyboard, Binding::Type BND=Binding::Type::numBindings, void (Program::*LCL)(Button*)=nullptr, void (Program::*RCL)(Button*)=nullptr, void (Program::*DCL)(Button*)=nullptr, Alignment ALG=Alignment::center, SDL_Texture* TEX=nullptr, int TMG=Default::textMargin, bool SBG=true, int MRG=Default::iconMargin, Layout* PNT=nullptr, sizt ID=SIZE_MAX);
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
