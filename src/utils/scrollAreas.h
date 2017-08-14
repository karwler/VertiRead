#pragma once

#include "objects.h"

// an abstact class for an area where you can scroll through stuff (usually Items)
class ScrollArea : public Object {
public:
	ScrollArea(const Object& BASE=Object());
	virtual ~ScrollArea();
	virtual ScrollArea* clone() const = 0;

	void dragSlider(int ypos);
	void dragList(int ypos);
	void scrollList(int ymov);

	virtual void tick(float dSec);
	virtual void setValues();		// requires listH to be set
	void setMotion(float val);
	int barW() const;				// returns 0 if slider isn't needed
	int getListY() const;
	virtual int getListH() const;
	virtual float getZoom() const;
	int getListL() const;
	int sliderY() const;
	int getSliderH() const;

	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;
	virtual btsel getSelectedItem() const;	// this class doesn't contain any items, therefore this function returns a false btsel
	virtual vec2t visibleItems() const;	// this class doesn't contain any items, therefore this function returns a false interval ([1, 0])
	
	ListItem* selectedItem;
	int diffSliderMouseY;	// space between slider and mouse position
protected:
	int listY;				// position of the list
	int sliderH;			// slider height
	int listH, listL;		// list height, max list position
	float motion;			// how much the list scrolls over time ()

	void checkListY();	// check if listY is out of limits and correct if so
};

// abstract class as base for working with items and placing them only on top of each other
class ScrollAreaItems : public ScrollArea {
public:
	ScrollAreaItems(const Object& BASE=Object());
	virtual ~ScrollAreaItems();
	virtual ScrollAreaItems* clone() const = 0;

	void setValues(size_t numRows);
	virtual vector<ListItem*> getItems() const = 0;

	virtual ListItem* item(size_t id) const = 0;
	virtual SDL_Rect itemRect(size_t id) const = 0;
	virtual SDL_Rect itemRect(size_t id, SDL_Rect& crop, EColor& color) const = 0;

	virtual btsel getSelectedItem() const = 0;
	virtual vec2t visibleItems() const = 0;
};

// places items on top of each other
class ListBox : public ScrollAreaItems {
public:
	ListBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={});
	virtual ~ListBox();
	virtual ListBox* clone() const;

	virtual void setValues();
	virtual vector<ListItem*> getItems() const;
	void setItems(const vector<ListItem*>& objects);
	virtual ListItem* item(size_t id) const;
	virtual SDL_Rect itemRect(size_t id) const;
	virtual SDL_Rect itemRect(size_t id, SDL_Rect& crop, EColor& color) const;
	virtual btsel getSelectedItem() const;
	virtual vec2t visibleItems() const;

private:
	vector<ListItem*> items;
};

// places items in a table
class TableBox : public ScrollAreaItems {
public:
	TableBox(const Object& BASE=Object(), const grid2<ListItem*>& ITMS=grid2<ListItem*>(), const vector<float>& IWS={});
	virtual ~TableBox();
	virtual TableBox* clone() const;

	virtual void setValues();
	virtual vector<ListItem*> getItems() const;
	const grid2<ListItem*>& getItemsGrid() const;
	void setItems(const grid2<ListItem*>& objects);

	virtual ListItem* item(size_t id) const;
	virtual SDL_Rect itemRect(size_t id) const;
	virtual SDL_Rect itemRect(size_t id, SDL_Rect& crop, EColor& color) const;

	virtual btsel getSelectedItem() const;
	virtual vec2t visibleItems() const;

private:
	vector<float> itemW;	// width of each coloumn. all elements should add up to 1
	grid2<ListItem*> items;
};

// places items as tiles one after another
class TileBox : public ScrollAreaItems {
public:
	TileBox(const Object& BASE=Object(), const vector<ListItem*>& ITMS={}, const vec2i& TS=vec2i(50, 50));
	virtual ~TileBox();
	virtual TileBox* clone() const;

	vec2i getTileSize() const;
	vec2i getDim() const;
	virtual void setValues();
	virtual vector<ListItem*> getItems() const;
	void setItems(const vector<ListItem*>& objects);

	virtual ListItem* item(size_t id) const;
	virtual SDL_Rect itemRect(size_t id) const;
	virtual SDL_Rect itemRect(size_t id, SDL_Rect& crop, EColor& color) const;
	virtual btsel getSelectedItem() const;
	virtual vec2t visibleItems() const;

private:
	vec2i tileSize;
	vec2i dim;		// amount of tiles per column/row
	vector<ListItem*> items;
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
public:
	ReaderBox(const Object& BASE=Object(), const vector<Texture*>& PICS={}, const string& CURPIC="", float ZOOM=1.f);
	virtual ~ReaderBox();
	virtual ReaderBox* clone() const;

	virtual void tick(float dSec);
	void onMouseMove(const vec2i& mPos);

	void dragListX(int xpos);
	void scrollListX(int xmov);
	void setZoom(float factor);
	void multZoom(float zfactor);
	void divZoom(float zfactor);

	virtual void setValues();
	int getListX() const;
	int getListW() const;
	virtual int getListH() const;
	virtual float getZoom() const;
	bool showMouse() const;
	bool showSlider() const;
	bool showList() const;
	bool showPlayer() const;

	const vector<Image>& getPictures() const;
	void setPictures(const vector<Texture*>& pictures, const string& curPic="");
	SDL_Rect listRect() const;
	const vector<Object*>& getListObjects() const;
	SDL_Rect playerRect() const;
	const vector<Object*>& getPlayerObjects();
	Image image(size_t id) const;
	Image image(size_t id, SDL_Rect& crop) const;
	virtual vec2t visibleItems() const;

private:
	bool mouseHideable;			// aka mouse not over slider, list or player
	float mouseTimer;			// time left until mouse disappeares
	float sliderTimer, listTimer, playerTimer;	// time until slider, button list or player disappear
	float zoom;					// current zoom factor
	int listX, listW, listXL;	// like listY, listH and listL except for horizontal value

	vector<Image> pics;
	vector<Object*> listObjects, playerObjects;

	void checkListX();		// same as CheckListY except for x
	bool checkMouseOverSlider(const vec2i& mPos);
	bool checkMouseOverList(const vec2i& mPos);
	bool checkMouseOverPlayer(const vec2i& mPos);
};
