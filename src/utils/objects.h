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
	Object(const vec2i& ANC=0, vec2i POS=-1, const vec2i& SIZ=0, EFix FIX=NO_FIX, EColor CLR=EColor::rectangle);
	virtual ~Object();
	virtual Object* Clone() const;
	
	SDL_Rect getRect() const;
	vec2i Anchor() const;
	void Anchor(const vec2i& newPos);
	vec2i Pos() const;
	void Pos(const vec2i& newPos);
	vec2i End() const;
	void End(const vec2i& newPos);
	vec2i Size() const;
	void Size(const vec2i& newSize);

	EColor color;
protected:
	EFix fix;
	vec2f anchor;
	vec2f pos, end;				// distance between boundries and anchor point
};

class Label : public Object {
public:
	Label(const Object& BASE=Object(), const string& TXT="");
	virtual ~Label();
	virtual Label* Clone() const;

	Text getText() const;

	string text;
};

class Button : public Object {
public:
	Button(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr);
	virtual ~Button();
	virtual Button* Clone() const;

	virtual void OnClick();
	void Callback(void (Program::*func)());

protected:
	void (Program::*callback)();
};

class ButtonText : public Button {
public:
	ButtonText(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr, const string& TXT="");
	virtual ~ButtonText();
	virtual ButtonText* Clone() const;

	Text getText() const;

	string text;
};

class ButtonImage : public Button {
public:
	ButtonImage(const Object& BASE=Object(), void (Program::*CALLB)()=nullptr, const vector<string>& TEXS={});
	virtual ~ButtonImage();
	virtual ButtonImage* Clone() const;

	virtual void OnClick();
	Image CurTex() const;

private:
	vector<Texture*> texes;
	uint curTex;
};

class Checkbox : public Object {
public:
	Checkbox(const Object& BASE=Object(), const string& TXT="", bool ON=false, void (Program::*CALLB)(bool)=nullptr, int SPC=5);
	virtual ~Checkbox();
	virtual Checkbox* Clone() const;

	void OnClick();
	SDL_Rect getButton() const;
	SDL_Rect getCheckbox(EColor* color=nullptr) const;
	Text getText() const;

private:
	string label;
	bool on;
	int spacing;
	void (Program::*callback)(bool);
};
