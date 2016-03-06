#pragma once

#include "items.h"

class Object {
public:
	Object(vec2i POS = vec2i(), vec2i SCL = vec2i(100, 100), EColor CLR = EColor::rectangle, EObjectState STAT = EObjectState::active);
	virtual ~Object();

	SDL_Rect getRect() const;
	vec2i Size() const;
	virtual void setSize(vec2i newSize);

	EObjectState state;
	vec2i pos;
	EColor color;

protected:
	vec2i size;
};

class Image : public Object {
public:
	Image(Object BASE=Object(), string TEXN="");
	virtual ~Image();

	string texname;
};

class TextBox : public Object {
public:
	TextBox(Object BASE = Object(), Text TXT = Text(), vec4i MRGN=vec4i(2, 2, 4, 4), int SPC=0);
	virtual ~TextBox();

	vector<Text> getLines() const;
	void setText(Text TXT);
	virtual void setSize(int val, bool byWidth);

	vec4i margin;
private:
	int spacing;
	Text text;

	vec2i CalculateSize(Text TXT) const;
};

class Button : public Object {
public:
	Button(Object BASE=Object(), void (Program::*CALLB)()=nullptr);
	virtual ~Button();

	virtual void OnClick();
	void setCallback(void (Program::*func)());

protected:
	void (Program::*callback)();
};

class ButtonImage : public Button {
public:
	ButtonImage(Object BASE = Object(), void (Program::*CALLB)() = nullptr, string TEXN = "");
	virtual ~ButtonImage();

	string texname;
};

class ButtonText : public Button {
public:
	ButtonText(Object BASE = Object(), void (Program::*CALLB)() = nullptr, string TXT="", EColor TCLR=EColor::text);
	virtual ~ButtonText();

	string text;
	EColor textColor;
};

class ScrollArea : public Object {
public:
	ScrollArea(Object BASE = Object(), int SPC = 5, int BARW = 10);
	virtual ~ScrollArea();

	void ScrollSlider(float mov);
	void ScrollList(int mov);

	SDL_Rect Bar() const;
	SDL_Rect Slider() const;

	int listY() const;
	int Spacing() const;

	int barW;
protected:
	int spacing;
	float sliderY;
	int sliderH, sliderL;
	int listH, listL;

	void SetScollValues();
};

class ListBox : public ScrollArea {
public:
	ListBox(Object BASE = Object(), const vector<ListItem*>& ITMS = vector<ListItem*>(), int SPC=5, int BARW=10);
	virtual ~ListBox();

	vector<ListItem*> Items() const;
	void setItems(const vector<ListItem*>& objects);

private:
	vector<ListItem*> items;
};

class TileBox : public ScrollArea {
public:
	TileBox(Object BASE = Object(), const vector<TileItem>& ITMS = vector<TileItem>(), vec2i TS = vec2i(50, 50), int SPC = 5, int BARW = 10);
	virtual ~TileBox();

	vector<TileItem>& Items();
	void setItems(const vector<TileItem>& objects);

	vec2i TileSize() const;
	int TilesPerRow() const;

private:
	vec2i tileSize;
	int tilesPerRow;

	vector<TileItem> items;
};

class ReaderBox : public Object {
public:
	ReaderBox(const vector<Image*>& PICS = vector<Image*>());
	virtual ~ReaderBox();

	vector<Image*> Pictures() const;
	void SetPictures(const vector<Image*>& pictures);

private:
	float zoom;
	vec2i listP;
	
	vector<Image*> pics;
};