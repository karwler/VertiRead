#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
public:
	Layout(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, bool VRT=true, int SPC=Default::spacing);
	virtual ~Layout();

	virtual void drawSelf();
	virtual void onResize();
	virtual void tick(float dSec);
	virtual void postInit();
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	virtual void onSelectUp() {}
	virtual void onSelectDown() {}
	virtual void onSelectLeft() {}
	virtual void onSelectRight() {}
	virtual bool selectable() const { return widgets.size(); }

	virtual void selectNext(sizt id, int mid, uint8 dir);	// 0 up, 1 down, 2 left, 3 right
	virtual void selectFrom(int mid, uint8 dir);	

	virtual Widget* getWidget(sizt id) const { return widgets[id]; }
	const vector<Widget*>& getWidgets() const { return widgets; }
	bool isVertical() const { return vertical; }
	virtual vec2i position() const;
	virtual vec2i size() const;
	virtual SDL_Rect parentFrame() const;
	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;

protected:
	vector<Widget*> widgets;
	vector<vec2i> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	int spacing;				// space between widgets
	bool vertical;				// whether to stack widgets vertically or horizontally

	virtual vec2i listSize() const;

	void selectWidget(sizt id, int mid, uint8 dir);
private:
	void scanSequential(sizt id, int mid, uint8 dir);
	void scanPerpendicular(int mid, uint8 dir);
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public Layout {
public:
	Popup(const vec2s& SIZ=vec2s(), const vector<Widget*>& WGS={}, bool VRT=true, int SPC=Default::spacing);
	virtual ~Popup() {}

	virtual void drawSelf();

	virtual vec2i position() const;
	virtual vec2i size() const;

private:
	Size sizeY;	// use Widget's relSize as sizeX
};

// popup that can be enabled or disabled
class Overlay : public Popup {
public:
	Overlay(const vec2s& POS=vec2s(), const vec2s& SIZ=vec2s(), const vec2s& APS=vec2s(), const vec2s& ASZ=vec2s(), const vector<Widget*>& WGS={}, bool VRT=true, int SPC=Default::spacing);
	virtual ~Overlay() {}

	virtual vec2i position() const;
	SDL_Rect actRect();

	bool on;
private:
	vec2s pos;
	vec2s actPos, actSize;	// positions and size of activation area
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
public:
	ScrollArea(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, int SPC=Default::spacing);
	virtual ~ScrollArea() {}

	virtual void drawSelf();
	virtual void onResize();
	virtual void tick(float dSec);
	virtual void onHold(const vec2i& mPos, uint8 mBut);
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov);
	virtual void onUndrag(uint8 mBut);
	virtual void onScroll(const vec2i& wMov);

	virtual void selectNext(sizt id, int mid, uint8 dir);
	virtual void selectFrom(int mid, uint8 dir);
	void scrollToWidgetPos(sizt id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(sizt id);

	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;
	virtual SDL_Rect frame() const { return rect(); }
	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;
	virtual vec2t visibleWidgets() const;
	
protected:
	void scrollToSelected();
	virtual vec2i listLim() const;	// max list position

	vec2i listPos;
	bool draggingSlider;
private:
	float motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position

	void setSlider(int ypos);
	int barWidth() const;	// returns 0 if slider isn't needed
	int sliderHeight() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position
};

// places items as tiles one after another
class TileBox : public ScrollArea {
public:
	TileBox(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, int WHT=Default::itemHeight, int SPC=Default::spacing);
	virtual ~TileBox() {}

	virtual void onResize();

	virtual void selectNext(sizt id, int mid, uint8 dir);	// 0 up, 1 down, 2 left, 3 right
	virtual void selectFrom(int mid, uint8 dir);

	virtual vec2i wgtSize(sizt id) const;
	virtual vec2t visibleWidgets() const;

private:
	int wheight;

	void scanVertically(sizt id, int mid, uint8 dir);
	void scanHorizontally(sizt id, int mid, uint8 dir);
	void scanFromStart(int mid, uint8 dir);
	void scanFromEnd(int mid, uint8 dir);
	void selectIfInRange(sizt id, int mid, uint8 dir);
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
public:
	ReaderBox(const Size& SIZ=Size(), const vector<Widget*>& PICS={}, int SPC=Default::spacing, float ZOOM=1.f);
	virtual ~ReaderBox() {}

	virtual void drawSelf();
	virtual void onResize();
	virtual void tick(float dSec);
	virtual void postInit();
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov);

	bool showBar() const;
	float getZoom() const { return zoom; }
	void setZoom(float factor);
	void centerListX();		// set listPos.x so that the view will be in the centter
	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;

private:
	bool countDown;		// whether to decrease cursorTimer until cursor hide
	float cursorTimer;	// time left until cursor/overlay disappeares
	float zoom;

	virtual vec2i listSize() const;
};
