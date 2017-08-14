#pragma once

#include "capturers.h"

// fix basically means "store value in pixels instead of a 'percentage'" (which would be a value between 0 and 1)
enum EFix : uint8 {
	FIX_NONE = 0x0,
	FIX_X    = 0x1,
	FIX_Y    = 0x2,
	FIX_ANC  = 0x3,		// fix anchor's position
	FIX_W    = 0x4,
	FIX_H    = 0x8,
	FIX_SIZ  = 0xC,		// keep distance between pos/end and anchor (don't resize)
	FIX_PX   = 0x10,
	FIX_PY   = 0x20,
	FIX_POS  = 0x30,	// start point won't be dependent on size (overrides fix_size flag when accessing start point)
	FIX_EX   = 0x40,
	FIX_EY   = 0x80,
	FIX_END  = 0xC0,	// end point won't be dependent on size (overrides fix_size flag when accessing end point)
	FIX_ALL  = 0xFF
};
EFix operator~(EFix a);
EFix operator&(EFix a, EFix b);
EFix operator&=(EFix& a, EFix b);
EFix operator^(EFix a, EFix b);
EFix operator^=(EFix& a, EFix b);
EFix operator|(EFix a, EFix b);
EFix operator|=(EFix& a, EFix b);

// basic UI element. an Object on it's own is displayed as a plain rectangle
class Object {
public:
	Object(const vec2i& ANC=0, vec2i POS=-1, const vec2i& SIZ=0, EFix FIX=FIX_NONE, EColor CLR=EColor::rectangle);
	virtual ~Object();
	virtual Object* clone() const;
	
	SDL_Rect rect() const;
	vec2i anchor() const;
	void setAnchor(const vec2i& newPos);
	vec2i pos() const;
	void setPos(const vec2i& newPos);
	vec2i end() const;
	void setEnd(const vec2i& newPos);
	vec2i size() const;
	void setSize(const vec2i& newSize);	// only modifies value of end, which means that it doesn't affect pos, aka. expands the rect to the right/bottom

	EColor color;
protected:
	EFix fix;			// how vanc, vpos and vend behave on window resize
	vec2f vanc;			// anchor point
	vec2f vpos, vend;	// distance between boundaries and anchor point
};

// Object with text
class Label : public Object {
public:
	Label(const Object& BASE=Object(), const string& TXT="", ETextAlign ALG=ETextAlign::left);
	virtual ~Label();
	virtual Label* clone() const;

	Text getText() const;

	ETextAlign align;
	string text;
};

// this class isn't supposed to be used on it's own since it's only a plain rectangle
class Button : public Object {
public:
	Button(const Object& BASE=Object(), void (Program::*CALL)()=nullptr);
	virtual ~Button();
	virtual Button* clone() const;

	virtual void onClick(ClickType click);
	void setCall(void (Program::*func)());

protected:
	void (Program::*call)();
};

// a button with one line of text with text alignment
class ButtonText : public Button {
public:
	ButtonText(const Object& BASE=Object(), void (Program::*CALL)()=nullptr, const string& LBL="", ETextAlign ALG=ETextAlign::left);
	virtual ~ButtonText();
	virtual ButtonText* clone() const;

	Text text() const;

	ETextAlign align;
	string label;
};

// can have multiple images through which it cycles when presed
class ButtonImage : public Button {
public:
	ButtonImage(const Object& BASE=Object(), void (Program::*CALL)()=nullptr, const vector<string>& TEXS={});
	virtual ~ButtonImage();
	virtual ButtonImage* clone() const;

	virtual void onClick(ClickType click);
	Image getCurTex() const;

private:
	vector<Texture*> texes;	// textures through which the button cycles on click
	size_t curTex;			// currently displayed texture
};

// like LineEdit except it's an Object on it's own rather than part of a ScrollArea
class LineEditor : public Object, public LineEdit {
public:
	LineEditor(const Object& BASE=Object(), const string& TXT="", ETextType TYPE=ETextType::text, void (Program::*KCALL)(const string&)=nullptr, void (Program::*CCALL)()=nullptr);
	virtual ~LineEditor();
	virtual LineEditor* clone() const;

	virtual Text text() const;
	virtual SDL_Rect caretRect() const;

private:
	virtual void checkCaretRight();
	virtual void checkCaretLeft();
};

// a popup window. should be handeled separately by Scene
class Popup : public Object {
public:
	Popup(const Object& BASE=Object(), const vector<Object*>& OBJS={});
	virtual ~Popup();
	virtual Popup* clone() const;

	vector<Object*> objects;
};
