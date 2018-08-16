#pragma once

#include "settings.h"

// size of a widget in pixels or relative to it's parent
struct Size {
	Size(int pixels);
	Size(float percent=1.f);

	bool usePix;
	union {
		int pix;	// use if type is pix
		float prc;	// use if type is prc
	};

	void set(int pxiels);
	void set(float precent);
};
using vec2s = vec2<Size>;

// can be used as spacer
class Widget {
public:
	Widget(const Size& relSize=Size(), Layout* parent=nullptr, sizt id=SIZE_MAX);	// parent and id should be set by Layout
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
	virtual void onNavSelect(const Direction& dir);
	virtual bool navSelectable() const { return false; }

	sizt getID() const { return pcID; }
	Layout* getParent() const { return parent; }
	void setParent(Layout* pnt, sizt id);

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
	Button(const Size& relSize=Size(), PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, SDL_Texture* background=nullptr, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~Button() {}

	virtual void drawSelf() override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onDoubleClick(const vec2i& mPos, uint8 mBut) override;
	virtual bool navSelectable() const override;
	
	Color color();
	vec2i texRes() const;
	virtual SDL_Rect texRect() const;

	SDL_Texture* tex;
	bool showBG;
	int margin;
protected:
	PCall lcall, rcall, dcall;
};

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	CheckBox(const Size& relSize=Size(), bool on=false, PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, SDL_Texture* background=nullptr, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~CheckBox() {}

	virtual void drawSelf() override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	SDL_Rect boxRect() const;
	Color boxColor() const;

	bool on;
};

// horizontal slider (maybe one day it'll be able to be vertical)
class Slider : public Button {
public:
	Slider(const Size& relSize=Size(), int value=0, int minimum=0, int maximum=255, PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, SDL_Texture* background=nullptr, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~Slider() {}

	virtual void drawSelf() override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onUndrag(uint8 mBut) override;

	int getVal() const { return val; }
	void setVal(int value);

	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;

private:
	int val, vmin, vmax;
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
	Label(const Size& relSize=Size(), const string& text="", PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, Alignment alignment=Alignment::left, SDL_Texture* background=nullptr, int textMargin=Default::textMargin, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~Label();

	virtual void drawSelf() override;
	virtual void postInit() override;

	const string& getText() const { return text; }
	virtual void setText(const string& str);
	SDL_Rect textRect() const;
	SDL_Rect textFrame() const;
	virtual SDL_Rect texRect() const override;
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
	SwitchBox(const Size& relSize=Size(), const vector<string>& options={}, const string& curOption="", PCall call=nullptr, Alignment alignment=Alignment::left, SDL_Texture* background=nullptr, int textMargin=Default::textMargin, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~SwitchBox() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

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
	LineEdit(const Size& relSize=Size(), const string& text="", PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, TextType type=TextType::text, SDL_Texture* background=nullptr, int textMargin=Default::textMargin, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~LineEdit() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onKeypress(const SDL_Keysym& key) override;
	virtual void onText(const string& str) override;

	const string& getOldText() const { return oldText; }
	virtual void setText(const string& str) override;
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
	KeyGetter(const Size& relSize=Size(), AcceptType type=AcceptType::keyboard, Binding::Type binding=Binding::Type::numBindings, PCall leftCall=nullptr, PCall rightCall=nullptr, PCall doubleCall=nullptr, Alignment alignment=Alignment::center, SDL_Texture* background=nullptr, int textMargin=Default::textMargin, bool showBackground=true, int backgroundMargin=Default::iconMargin, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~KeyGetter() {}

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onKeypress(const SDL_Keysym& key) override;
	virtual void onJButton(uint8 jbutton) override;
	virtual void onJHat(uint8 jhat, uint8 value) override;
	virtual void onJAxis(uint8 jaxis, bool positive) override;
	virtual void onGButton(SDL_GameControllerButton gbutton) override;
	virtual void onGAxis(SDL_GameControllerAxis gaxis, bool positive) override;

private:
	AcceptType acceptType;		// what kind of binding is being accepted
	Binding::Type bindingType;	// index of what is currently being edited
};
