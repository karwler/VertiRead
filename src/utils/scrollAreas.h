#pragma once

#include "objects.h"

class ScrollArea : public Object {
public:
	ScrollArea(const Object& BASE=Object(), int SPC=5, int BARW=10);
	virtual ~ScrollArea();

	void DragSlider(int ypos);
	void DragList(int ypos);
	void ScrollList(int ymov);
	virtual void SetValues();

	SDL_Rect Bar() const;
	SDL_Rect Slider() const;

	int Spacing() const;
	int ListY() const;
	int SliderY() const;	// get global slider position in pixels
	int SliderH() const;

	int barW;
	int diffSliderMouseY;
protected:
	int spacing;
	int listY;
	int sliderH;
	int listH, listL;

	void SetScrollValues();	// needs listH and size.y in order to calculate listL, sliderH
};

class ListBox : public ScrollArea {
public:
	ListBox(const Object& BASE = Object(), const vector<ListItem*>& ITMS = vector<ListItem*>(), int SPC=5, int BARW=10);
	virtual ~ListBox();

	virtual void SetValues();
	const vector<ListItem*>& Items() const;
	void Items(const vector<ListItem*>& objects);

private:
	vector<ListItem*> items;
};

class TileBox : public ScrollArea {
public:
	TileBox(const Object& BASE = Object(), const vector<TileItem>& ITMS = vector<TileItem>(), vec2i TS = vec2i(50, 50), int SPC = 5, int BARW = 10);
	virtual ~TileBox();

	virtual void SetValues();
	vector<TileItem>& Items();
	void Items(const vector<TileItem>& objects);

	vec2i TileSize() const;
	int TilesPerRow() const;

private:
	vec2i tileSize;
	int tilesPerRow;

	vector<TileItem> items;
};

class ReaderBox : public ScrollArea {
public:
	ReaderBox(const vector<string>& PICS=vector<string>(), float ZOOM=1.f);
	virtual ~ReaderBox();

	void Tick();
	void DragListX(int xpos);
	void ScrollListX(int xmov);
	void Zoom(float factor);
	void AddZoom(float zadd);

	SDL_Rect List() const;	// return value is the background rect
	vector<ButtonImage>& ListButtons();
	SDL_Rect Player() const;
	vector<ButtonImage>& PlayerButtons();

	virtual void SetValues();
	const vector<Image>& Pictures() const;
	void Pictures(const vector<string>& pictures);
	int ListX() const;

	bool showSlider() const;
	bool showList() const;
	bool showPlayer() const;

	bool sliderFocused;
private:
	const float hideDelay;
	float sliderTimer, listTimer, playerTimer;
	float zoom;
	int listX, listXL;
	const int blistW, playerW;

	vector<Image> pics;
	vector<ButtonImage> listButtons;
	vector<ButtonImage> playerButtons;

	bool CheckMouseOverSlider();
	bool CheckMouseOverList();
	bool CheckMouseOverPlayer();
};
