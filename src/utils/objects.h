#pragma once

#include "items.h"

// if variable is an int, it's value is in pixels
// if variable is a float, it's value is 0 - 1 (1 = window resolution)

class Object {
public:
	Object(vec2f POS = vec2f(), vec2f SIZ = vec2f(0.2f, 0.2f), EColor CLR = EColor::rectangle);
	Object(vec2i POS, vec2i SIZ, EColor CLR = EColor::rectangle);
	virtual ~Object();

	SDL_Rect getRect() const;
	vec2i Pos() const;
	vec2i Size() const;

	vec2f pos;
	vec2f size;
	EColor color;
};

class TextBox : public Object {
public:
	TextBox(Object BASE = Object(), Text TXT = Text(), vec4i MRGN=vec4i(2, 2, 4, 4), int SPC=0);
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

	void DragSlider(int ypos);
	void DragList(int ypos);
	void ScrollList(int ymov);

	SDL_Rect Bar() const;
	SDL_Rect Slider() const;

	int Spacing() const;
	int ListY() const;
	float sliderY() const;	// calculate slider position
	int SliderY() const;	// get global slider position in pixels
	int SliderH() const;

	int barW;
	int diffSliderMouseY;
protected:
	int spacing;
	int listY;
	float sliderH;
	int listH, listL;

	void SetScollValues();	// needs listH and size.y in order to calculate listL, sliderH
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

class ReaderBox : public ScrollArea {
public:
	ReaderBox(const vector<string>& PICS = vector<string>(), float ZOOM=1.f);
	virtual ~ReaderBox();

	void DragListX(int xpos);
	void ScrollListX(int xmov);
	void Zoom(float factor);
	void AddZoom(float zadd);

	const vector<Image>& Pictures() const;
	void SetPictures(const vector<string>& pictures, float zoomFactor=1.f);
	int ListX() const;

	bool sliderFocused;
private:
	bool showSlider, showButtons, showPlayer;
	float zoom;
	int listX, listXL;
	
	vector<Image> pics;
};
