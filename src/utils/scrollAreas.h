#pragma once

#include "objects.h"

class ScrollArea : public Object {
public:
	ScrollArea(const Object& BASE=Object());
	virtual ~ScrollArea();
	virtual ScrollArea* Clone() const = 0;

	void DragSlider(int ypos);
	void DragList(int ypos);
	void ScrollList(int ymov);

	virtual void Tick(float dSec);
	virtual void SetValues();		// requires listH to be set
	void SetMotion(float val);
	int BarW() const;				// returns 0 if slider isn't needed
	int ListY() const;
	virtual int ListH() const;
	virtual float Zoom() const;
	int ListL() const;
	int SliderY() const;
	int SliderH() const;

	SDL_Rect Bar() const;
	SDL_Rect Slider() const;
	virtual btsel SelectedItem() const;	// this class doesn't contain any items, therefore this function returns a false btsel
	virtual vec2t VisibleItems() const;	// this class doesn't contain any items, therefore this function returns a false interval ([1, 0])
	
	ListItem* selectedItem;
	int diffSliderMouseY;	// space between slider and mouse position
protected:
	int listY;				// position of the list
	int sliderH;			// slider height
	int listH, listL;		// list height, max list position
	float motion;			// how much the list scrolls over time ()

	void CheckListY();	// check if listY is out of limits and correct if so
};

class ScrollAreaX1 : public ScrollArea {
public:
	ScrollAreaX1(const Object& BASE=Object());
	virtual ~ScrollAreaX1();
	virtual ScrollAreaX1* Clone() const = 0;

	virtual ListItem* Item(size_t id) const = 0;
	virtual SDL_Rect ItemRect(size_t id) const = 0;

protected:
	void SetValuesX1(size_t numRows);
};

class ListBox : public ScrollAreaX1 {
public:
	ListBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={});
	virtual ~ListBox();
	virtual ListBox* Clone() const;

	virtual void SetValues();
	const vector<ListItem*>& Items() const;
	void Items(const vector<ListItem*>& objects);
	virtual ListItem* Item(size_t id) const;
	virtual SDL_Rect ItemRect(size_t id) const;
	SDL_Rect ItemRect(size_t id, SDL_Rect& crop, EColor& color) const;
	virtual btsel SelectedItem() const;
	virtual vec2t VisibleItems() const;

private:
	vector<ListItem*> items;
};

class TableBox : public ScrollAreaX1 {
public:
	TableBox(const Object& BASE=Object(), const grid2<ListItem*>& ITMS=grid2<ListItem*>(), const vector<float>& IWS={});
	virtual ~TableBox();
	virtual TableBox* Clone() const;

	virtual void SetValues();
	const grid2<ListItem*>& Items() const;
	void Items(const grid2<ListItem*>& objects);
	virtual ListItem* Item(size_t id) const;
	virtual SDL_Rect ItemRect(size_t id) const;
	SDL_Rect ItemRect(size_t id, SDL_Rect& crop) const;
	virtual btsel SelectedItem() const;
	virtual vec2t VisibleItems() const;

private:
	vector<float> itemW;
	grid2<ListItem*> items;
};

class TileBox : public ScrollArea {
public:
	TileBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={}, const vec2i& TS=vec2i(50, 50));
	virtual ~TileBox();
	virtual TileBox* Clone() const;

	vec2i TileSize() const;
	vec2i Dim() const;

	const vector<ListItem*>& Items() const;
	void Items(const vector<ListItem*>& objects);
	SDL_Rect ItemRect(size_t id) const;
	SDL_Rect ItemRect(size_t id, SDL_Rect& crop, EColor& color) const;
	virtual btsel SelectedItem() const;
	virtual vec2t VisibleItems() const;

private:
	vec2i tileSize;
	vec2i dim;
	vector<ListItem*> items;

	virtual void SetValues();
};

class ReaderBox : public ScrollArea {
public:
	ReaderBox(const Object& BASE=Object(), const vector<Texture*>& PICS={}, const string& CURPIC="", float ZOOM=1.f);
	virtual ~ReaderBox();
	virtual ReaderBox* Clone() const;

	virtual void Tick(float dSec);
	void OnMouseMove(const vec2i& mPos);

	void DragListX(int xpos);
	void ScrollListX(int xmov);
	void Zoom(float factor);
	void MultZoom(float zfactor);
	void DivZoom(float zfactor);

	virtual void SetValues();
	int ListX() const;
	int ListW() const;
	virtual int ListH() const;
	virtual float Zoom() const;
	bool showMouse() const;
	bool showSlider() const;
	bool showList() const;
	bool showPlayer() const;

	const vector<Image>& Pictures() const;
	void Pictures(const vector<Texture*>& pictures, const string& curPic="");
	SDL_Rect List() const;		// return value is the background rect
	vector<Object*>& ListObjects();
	SDL_Rect Player() const;
	vector<Object*>& PlayerObjects();
	Image getImage(size_t id) const;
	Image getImage(size_t id, SDL_Rect& crop) const;
	virtual vec2t VisibleItems() const;

private:
	bool mouseHideable;		// aka mouse not over slider, list or player
	float mouseTimer, sliderTimer, listTimer, playerTimer;
	float zoom;
	int listX, listW, listXL;
	const int blistW, playerW, playerH;

	vector<Image> pics;
	vector<Object*> listObjects, playerObjects;

	void CheckListX();			// same as CheckListY
	bool CheckMouseOverSlider(const vec2i& mPos);
	bool CheckMouseOverList(const vec2i& mPos);
	bool CheckMouseOverPlayer(const vec2i& mPos);
};
