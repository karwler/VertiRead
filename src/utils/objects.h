#pragma once

#include "items.h"

enum EFixation : byte {
	FIX_NONE = 0x0,
	FIX_SX = 0x1,
	FIX_SY = 0x2,
	FIX_EX = 0x4,
	FIX_EY = 0x8
};
EFixation operator|(EFixation a, EFixation b);

// if variable is an int, it's value is in pixels
// if variable is a float, it's value is 0 - 1 (1 = window resolution)

class Object {
public:
	Object(vec2i POS=vec2i(), vec2i SIZ=vec2i(), EFixation FIX=FIX_NONE, EColor CLR = EColor::rectangle);
	virtual ~Object();

	template <typename T>
	bool isA() const {
		return dynamic_cast<T*>(const_cast<Object*>(this)) != nullptr;
	}

	SDL_Rect getRect() const;
	vec2i Pos() const;
	void Pos(vec2i newPos);
	vec2i End() const;
	void End(vec2i newEnd);
	vec2i Size() const;
	void Size(vec2i newSize);

	EColor color;
private:
	vec2f pos, end;
	EFixation fix;
};

class TextBox : public Object {
public:
	TextBox(const Object& BASE=Object(), const Text& TXT = Text(), vec4i MRGN=vec4i(2, 2, 4, 4), int SPC=0);
	virtual ~TextBox();

	vector<Text> getLines() const;
	void setText(Text TXT);
	void setSize(int val, bool byWidth);

	vec4i margin;
private:
	int spacing;
	Text text;

	vec2i CalculateSize() const;
};

class Button : public Object {
public:
	Button(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr);
	virtual ~Button();

	virtual void OnClick();
	void Callback(void (Program::*func)());

protected:
	void (Program::*callback)();
};

class ButtonImage : public Button {
public:
	ButtonImage(const Object& BASE = Object(), void (Program::*CALLB)() = nullptr, string TEXN = "");
	virtual ~ButtonImage();

	string texname;
};

class ButtonText : public Button {
public:
	ButtonText(const Object& BASE = Object(), void (Program::*CALLB)() = nullptr, string TXT="", EColor TCLR=EColor::text);
	virtual ~ButtonText();

	string text;
	EColor textColor;
};
