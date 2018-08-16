#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
public:
	enum class Select : uint8 {
		none,
		one,
		any
	};

	Layout(const Size& relSize=Size(), const vector<Widget*>& children={}, const Direction& direction=Direction::down, Select select=Select::none, int spacing=Default::spacing, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~Layout();

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onNavSelect(const Direction& dir) override {}
	virtual bool navSelectable() const override { return widgets.size(); }

	virtual void navSelectNext(sizt id, int mid, const Direction& dir);
	virtual void navSelectFrom(int mid, const Direction& dir);

	Widget* getWidget(sizt id) const { return widgets[id]; }
	const vector<Widget*>& getWidgets() const { return widgets; }
	const Direction& getDirection() const { return direction; }
	const uset<Widget*> getSelected() const { return selected; }
	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual SDL_Rect frame() const override;
	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;
	void selectWidget(sizt id);

protected:
	vector<Widget*> widgets;
	vector<vec2i> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	uset<Widget*> selected;
	Direction direction;		// in what direction widgets are stacked (TileBox doesn't support horizontal)
	Select selection;			// how many items can be selected at once
	int spacing;				// space between widgets

	virtual vec2i listSize() const;

	void navSelectWidget(sizt id, int mid, const Direction& dir);
private:
	void scanSequential(sizt id, int mid, const Direction& dir);
	void scanPerpendicular(int mid, const Direction& dir);

	void selectSingleWidget(sizt id);
	vec2t findMinMaxSelectedID() const;
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public Layout {
public:
	Popup(const vec2s& relSize=vec2s(), const vector<Widget*>& children={}, const Direction& direction=Direction::down, int spacing=Default::spacing);
	virtual ~Popup() {}

	virtual void drawSelf() override;

	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual SDL_Rect frame() const override;

private:
	Size sizeY;	// use Widget's relSize as sizeX
};

// popup that can be enabled or disabled
class Overlay : public Popup {
public:
	Overlay(const vec2s& position=vec2s(), const vec2s& relSize=vec2s(), const vec2s& activationPos=vec2s(), const vec2s& activationSize=vec2s(), const vector<Widget*>& children={}, const Direction& direction=Direction::down, int spacing=Default::spacing);
	virtual ~Overlay() {}

	virtual vec2i position() const override;
	SDL_Rect actRect() const;

	bool on;
private:
	vec2s pos;
	vec2s actPos, actSize;	// positions and size of activation area
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
public:
	ScrollArea(const Size& relSize=Size(), const vector<Widget*>& children={}, const Direction& direction=Direction::down, Select select=Select::none, int spacing=Default::spacing, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~ScrollArea() {}

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(const vec2i& wMov) override;

	virtual void navSelectNext(sizt id, int mid, const Direction& dir) override;
	virtual void navSelectFrom(int mid, const Direction& dir) override;
	void scrollToWidgetPos(sizt id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(sizt id);

	virtual SDL_Rect frame() const;
	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;
	SDL_Rect barRect() const;
	SDL_Rect sliderRect() const;
	vec2t visibleWidgets() const;
	void moveListPos(const vec2i& mov);
	
protected:
	void scrollToSelected();
	virtual vec2i listLim() const;	// max list position
	virtual int wgtRPos(sizt id) const;
	virtual int wgtREnd(sizt id) const;

	vec2i listPos;
	bool draggingSlider;
private:
	vec2f motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position

	void setSlider(int ypos);
	int barSize() const;	// returns 0 if slider isn't needed
	int sliderSize() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position
	void throttleMotion(float& mov, float dSec);
};

// places items as tiles one after another
class TileBox : public ScrollArea {
public:
	TileBox(const Size& relSize=Size(), const vector<Widget*>& children={}, int childHeight=Default::itemHeight, const Direction& direction=Direction::down, Select select=Select::none, int spacing=Default::spacing, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~TileBox() {}

	virtual void onResize() override;

	virtual void navSelectNext(sizt id, int mid, const Direction& dir) override;
	virtual void navSelectFrom(int mid, const Direction& dir) override;

	virtual vec2i wgtSize(sizt id) const override;

private:
	int wheight;

	void scanVertically(sizt id, int mid, const Direction& dir);
	void scanHorizontally(sizt id, int mid, const Direction& dir);
	void scanFromStart(int mid, const Direction& dir);
	void scanFromEnd(int mid, const Direction& dir);
	void navSelectIfInRange(sizt id, int mid, const Direction& dir);

	virtual int wgtREnd(sizt id) const override;
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
public:
	ReaderBox(const Size& relSize=Size(), const Direction& direction=Direction::down, int spacing=Default::spacing, Layout* parent=nullptr, sizt id=SIZE_MAX);
	virtual ~ReaderBox();

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov) override;

	bool showBar() const;
	float getZoom() const { return zoom; }
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the centter
	const string& getFile(sizt id) const { return pics[id].first; }
	virtual vec2i wgtPosition(sizt id) const override;
	virtual vec2i wgtSize(sizt id) const override;

private:
	vector<pair<string, SDL_Texture*>> pics;
	bool countDown;			// whether to decrease cursorTimer until cursor hide
	float cursorTimer;		// time left until cursor/overlay disappeares
	float zoom;

	virtual vec2i listSize() const;
	virtual int wgtRPos(sizt id) const override;
	virtual int wgtREnd(sizt id) const override;

	vec2i texRes(sizt id) const;
};
