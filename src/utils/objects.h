#pragma once

#include "items.h"

class Object {
public:
	Object(vec2i ANC=vec2i(), vec2i POS=vec2i(-1, -1), vec2i SIZ=vec2i(), bool FX=false, bool FY=false, bool KW=false, bool KH=false, EColor CLR = EColor::rectangle);
	virtual ~Object();

	template <typename T>
	bool isA() const {
		return dynamic_cast<T*>(const_cast<Object*>(this)) != nullptr;
	}

	SDL_Rect getRect() const;
	vec2i Anchor() const;
	void Anchor(vec2i newPos);
	vec2i Pos() const;
	void Pos(vec2i newPos);
	vec2i End() const;
	void End(vec2i newPos);
	vec2i Size() const;
	void Size(vec2i newSize);

	EColor color;
private:
	vec2f anchor;
	vec2f pos, end;				// distance between boundries and anchor point
	bool fixX, fixY;			// fix anchors position
	bool keepWidth, keepHeight;	// keep distance between pos/end and anchor (don't resize)
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
	ButtonImage(const Object& BASE = Object(), void (Program::*CALLB)() = nullptr, const vector<string>& TEXS=vector<string>());
	virtual ~ButtonImage();

	virtual void OnClick();
	string CurTex() const;

private:
	vector<string> texes;
	uint curTex;
};

class ButtonText : public Button {
public:
	ButtonText(const Object& BASE = Object(), void (Program::*CALLB)() = nullptr, string TXT="", EColor TCLR=EColor::text);
	virtual ~ButtonText();

	string text;
	EColor textColor;
};
