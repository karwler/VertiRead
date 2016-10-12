#pragma once

#include "objects.h"

class ScrollArea : public Object {
public:
	ScrollArea(const Object& BASE=Object(), int SPC=5, int BARW=10);
	virtual ~ScrollArea();
	virtual ScrollArea* Clone() const = 0;

	virtual void SetValues();
	void DragSlider(int ypos);
	void DragList(int ypos);
	void ScrollList(int ymov);

	SDL_Rect Bar() const;
	SDL_Rect Slider() const;
	virtual int SelectedItem() const;	// this class doesn't contain any items, therefore this function returns -1

	int BarW() const;					// returns 0 if slider isn't needed
	int ListY() const;
	virtual int ListH() const;
	int ListL() const;
	int SliderY() const;
	int SliderH() const;

	ListItem* selectedItem;
	
	const int spacing;
	int diffSliderMouseY;
protected:
	const int barW;
	int listY;
	int sliderH;
	int listH, listL;

	void CheckListY();		// check if listY is out of limits and correct if so
};

class ListBox : public ScrollArea {
public:
	ListBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={}, int IH=30, int SPC=5, int BARW=10);
	virtual ~ListBox();
	virtual ListBox* Clone() const;

	ListItem* Item(int id) const;
	SDL_Rect ItemRect(int i, SDL_Rect* Crop=nullptr, EColor* color=nullptr) const;
	virtual int SelectedItem() const;
	vec2t VisibleItems() const;

	const vector<ListItem*>& Items() const;
	void Items(const vector<ListItem*>& objects);
	int ItemH() const;

private:
	int itemH;
	vector<ListItem*> items;
};

class TileBox : public ScrollArea {
public:
	TileBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={}, const vec2i& TS=vec2i(50, 50), int BARW=10);
	virtual ~TileBox();
	virtual TileBox* Clone() const;

	virtual void SetValues();
	SDL_Rect ItemRect(int id, SDL_Rect* Crop=nullptr, EColor* color=nullptr) const;
	virtual int SelectedItem() const;
	vec2t VisibleItems() const;

	const vector<ListItem*>& Items() const;
	void Items(const vector<ListItem*>& objects);
	vec2i TileSize() const;
	vec2i Dim() const;

private:
	vec2i tileSize;
	vec2i dim;

	vector<ListItem*> items;
};

class ReaderBox : public ScrollArea {
public:
	ReaderBox(const Object& BASE=Object(0, 0, 100, FIX_ANC | FIX_END, EColor::background), const vector<Texture*>& PICS={}, const string& CURPIC="", float ZOOM=1.f);
	virtual ~ReaderBox();
	virtual ReaderBox* Clone() const;

	virtual void SetValues();
	void Tick(float dSec);
	void DragListX(int xpos);
	void ScrollListX(int xmov);
	void Zoom(float factor);
	void MultZoom(float zfactor);
	void DivZoom(float zfactor);

	SDL_Rect List() const;	// return value is the background rect
	vector<ButtonImage>& ListButtons();
	SDL_Rect Player() const;
	vector<ButtonImage>& PlayerButtons();
	Image getImage(int i, SDL_Rect* Crop=nullptr) const;
	vec2t VisiblePictures() const;

	const vector<Image>& Pictures() const;
	void Pictures(const vector<Texture*>& pictures, const string& curPic="");
	int ListX() const;
	int ListW() const;
	virtual int ListH() const;
	bool showMouse() const;
	bool showSlider() const;
	bool showList() const;
	bool showPlayer() const;

	bool sliderFocused;
private:
	const float hideDelay;
	float mouseTimer, sliderTimer, listTimer, playerTimer;
	float zoom;
	int listX, listW, listXL;
	const int blistW, playerW;

	vector<Image> pics;
	vector<ButtonImage> listButtons;
	vector<ButtonImage> playerButtons;

	void CheckMouseShow(float dSec);
	void CheckMouseOverSlider(float dSec);
	void CheckMouseOverList(float dSec);
	void CheckMouseOverPlayer(float dSec);
	void CheckListX();			// same as CheckListY
};
