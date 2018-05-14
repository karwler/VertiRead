#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
public:
	enum class Selection : uint8 {
		none,
		one,
		any
	};

	Layout(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, bool VRT=true, Selection SLT=Selection::none, int SPC=Default::spacing);
	virtual ~Layout();

	virtual void drawSelf();
	virtual void onResize();
	virtual void tick(float dSec);
	virtual void postInit();
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov);

	virtual Widget* getWidget(sizt id) const { return widgets[id]; }
	const vector<Widget*>& getWidgets() const { return widgets; }
	const uset<Widget*>& getSelected() const { return selected; }
	void selectWidget(Widget* wgt);	// returns true if an interaction has occured

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
	Selection selection;		// how many children can be selected
	uset<Widget*> selected;		// currently selected children

	vec2i listSize() const;
private:
	void selectSingle(Widget* wgt);
	void selectAnother(Widget* wgt);
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public Layout {
public:
	Popup(const vec2s& SIZ=vec2s(), const vector<Widget*>& WGS={}, bool VRT=true, Selection SLT=Selection::none, int SPC=Default::spacing);
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
	Overlay(const vec2s& POS=vec2s(), const vec2s& SIZ=vec2s(), const vec2s& APS=vec2s(), const vec2s& ASZ=vec2s(), const vector<Widget*>& WGS={}, bool VRT=true, Selection SLT=Selection::none, int SPC=Default::spacing);
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
	ScrollArea(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, Selection SLT=Selection::none, int SPC=Default::spacing);
	virtual ~ScrollArea() {}

	virtual void drawSelf();
	virtual void onResize();
	virtual void tick(float dSec);
	virtual void onHold(const vec2i& mPos, uint8 mBut);
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov);
	virtual void onUndrag(uint8 mBut);
	virtual void onScroll(const vec2i& wMov);

	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;
	virtual SDL_Rect frame() const { return rect(); }
	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;
	virtual vec2t visibleWidgets() const;
	
protected:
	virtual vec2i listLim() const;	// max list position

	vec2i listPos;
	bool draggingSlider;
private:
	vec2f motion;			// how much the list scrolls over time
	int diffSliderMouseY;	// space between slider and mouse position

	void throttleMotion(float& mot, float dSec);
	void setSlider(int ypos);

	int barWidth() const;	// returns 0 if slider isn't needed
	int sliderHeight() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position
};

// places items as tiles one after another
class TileBox : public ScrollArea {
public:
	TileBox(const Size& SIZ=Size(), const vector<Widget*>& WGS={}, Selection SLT=Selection::none, int WHT=Default::itemHeight, int SPC=Default::spacing);
	virtual ~TileBox() {}

	virtual void onResize();

	virtual vec2i wgtSize(sizt id) const;
	virtual vec2t visibleWidgets() const;

private:
	int wheight;
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
	void centerListX();

	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;

	float zoom;
private:
	bool cursorFree;	// if mouse not over menu or scroll bar
	float cursorTimer;	// time left until cursor/overlay disappeares

	virtual vec2i listLim() const;
};
