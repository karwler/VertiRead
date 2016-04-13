#pragma once

#include "items.h"

enum EFix : byte {
	NO_FIX  = 0x0,
	FIX_X   = 0x1,
	FIX_Y   = 0x2,
	FIX_POS = 0x3,	// fix anchors position
	FIX_W   = 0x4,
	FIX_H   = 0x8,
	FIX_SIZ = 0xC,	// keep distance between pos/end and anchor (don't resize)
	FIX_EX  = 0x10,
	FIX_EY  = 0x20,
	FIX_END = 0x30,	// if set end point won't be dependent on size (overrides fix_size flag when accessing end point)
	FIX_ALL = 0xFF
};
EFix operator|(EFix a, EFix b);

class Object {
public:
	Object(vec2i ANC=0, vec2i POS=vec2i(-1, -1), vec2i SIZ=0, EFix FIX=NO_FIX, EColor CLR = EColor::rectangle);
	virtual ~Object();

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
protected:
	EFix fix;
	vec2f anchor;
	vec2f pos, end;				// distance between boundries and anchor point
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
	ButtonImage(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr, const vector<string>& TEXS=vector<string>());
	virtual ~ButtonImage();

	virtual void OnClick();
	Image CurTex() const;

private:
	vector<Texture*> texes;
	uint curTex;
};

class ButtonText : public Button {
public:
	ButtonText(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr, string TXT="");
	virtual ~ButtonText();

	Text getText() const;
private:
	string label;
};
