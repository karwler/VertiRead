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

	Layout(const Size& relSize = Size(), const vector<Widget*>& children = {}, const Direction& direction = Direction::down, Select select = Select::none, int spacing = Default::itemSpacing, Layout* parent = nullptr, sizt id = SIZE_MAX);
	virtual ~Layout() override;

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onNavSelect(const Direction&) override {}
	virtual bool navSelectable() const override;

	virtual void navSelectNext(sizt id, int mid, const Direction& dir);
	virtual void navSelectFrom(int mid, const Direction& dir);

	Widget* getWidget(sizt id) const;
	const vector<Widget*>& getWidgets() const;
	const uset<Widget*> getSelected() const;
	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual Rect frame() const override;
	virtual vec2i wgtPosition(sizt id) const;
	virtual vec2i wgtSize(sizt id) const;
	void selectWidget(sizt id);

protected:
	vector<Widget*> widgets;
	vector<vec2i> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	uset<Widget*> selected;
	Select selection;			// how many items can be selected at once
	Direction direction;		// in what direction widgets are stacked (TileBox doesn't support horizontal)
	int spacing;				// space between widgets

	virtual vec2i listSize() const;

	void navSelectWidget(sizt id, int mid, const Direction& dir);
private:
	void scanSequential(sizt id, int mid, const Direction& dir);
	void scanPerpendicular(int mid, const Direction& dir);

	void selectSingleWidget(sizt id);
	vec2t findMinMaxSelectedID() const;
};

inline Widget* Layout::getWidget(sizt id) const {
	return widgets[id];
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline const uset<Widget*> Layout::getSelected() const {
	return selected;
}

// layout with background with free position/size (shouldn't have a parent)
class Popup : public Layout {
public:
	Popup(const vec2s& relSize = vec2s(), const vector<Widget*>& children = {}, const Direction& direction = Direction::down, int spacing = Default::itemSpacing);
	virtual ~Popup() override {}

	virtual void drawSelf() override;

	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual Rect frame() const override;

private:
	Size sizeY;	// use Widget's relSize as sizeX
};

// popup that can be enabled or disabled
class Overlay : public Popup {
public:
	Overlay(const vec2s& position = vec2s(), const vec2s& relSize = vec2s(), const vec2s& activationPos = vec2s(), const vec2s& activationSize = vec2s(), const vector<Widget*>& children = {}, const Direction& direction = Direction::down, int spacing = Default::itemSpacing);
	virtual ~Overlay() override {}

	virtual vec2i position() const override;
	Rect actRect() const;

	bool on;
private:
	vec2s pos;
	vec2s actPos, actSize;	// positions and size of activation area
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
public:
	ScrollArea(const Size& relSize = Size(), const vector<Widget*>& children = {}, const Direction& direction = Direction::down, Select select = Select::none, int spacing = Default::itemSpacing, Layout* parent = nullptr, sizt id = SIZE_MAX);
	virtual ~ScrollArea() override {}

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(const vec2i& wMov) override;

	virtual void navSelectNext(sizt id, int mid, const Direction& dir) override;
	virtual void navSelectFrom(int mid, const Direction& dir) override;
	void scrollToWidgetPos(sizt id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(sizt id);
	bool scrollToNext();				// scroll to next widget (returns false if at scroll limit)
	bool scrollToPrevious();			// scroll to previous widget (returns false if at scroll limit)
	void scrollToLimit(bool start);		// scroll to start or end of the list relative to it's direction

	virtual Rect frame() const override;
	virtual vec2i wgtPosition(sizt id) const override;
	virtual vec2i wgtSize(sizt id) const override;
	Rect barRect() const;
	Rect sliderRect() const;
	vec2t visibleWidgets() const;
	void moveListPos(const vec2i& mov);
	
protected:
	void scrollToSelected();
	virtual vec2i listLim() const;	// max list position
	virtual int wgtRPos(sizt id) const;
	virtual int wgtREnd(sizt id) const;

	bool draggingSlider;
	vec2i listPos;
private:
	vec2f motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position

	void scrollToFollowing(sizt id, bool prev);
	void setSlider(int ypos);
	int barSize() const;	// returns 0 if slider isn't needed
	int sliderSize() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position
	static void throttleMotion(float& mov, float dSec);
};

inline void ScrollArea::moveListPos(const vec2i& mov) {
	listPos = bringIn(listPos + mov, vec2i(0), listLim());
}

inline int ScrollArea::sliderLim() const {
	return size()[direction.vertical()] - sliderSize();
}

// places items as tiles one after another
class TileBox : public ScrollArea {
public:
	TileBox(const Size& relSize = Size(), const vector<Widget*>& children = {}, int childHeight = Default::itemHeight, const Direction& direction = Direction::down, Select select = Select::none, int spacing = Default::itemSpacing, Layout* parent = nullptr, sizt id = SIZE_MAX);
	virtual ~TileBox() override {}

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
	ReaderBox(const Size& relSize = Size(), const Direction& direction = Direction::down, float zoom = Default::zoom, int spacing = Default::spacing, Layout* parent = nullptr, sizt id = SIZE_MAX);
	virtual ~ReaderBox() override;

	virtual void drawSelf() override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onMouseMove(const vec2i& mPos, const vec2i& mMov) override;

	bool showBar() const;
	float getZoom() const;
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the centter
	string curPage() const;
	virtual vec2i wgtPosition(sizt id) const override;
	virtual vec2i wgtSize(sizt id) const override;

private:
	vector<Texture> pics;
	float cursorTimer;		// time left until cursor/overlay disappeares
	float zoom;
	bool countDown;			// whether to decrease cursorTimer until cursor hide

	virtual vec2i listSize() const override;
	virtual int wgtRPos(sizt id) const override;
	virtual int wgtREnd(sizt id) const override;
};

inline bool ReaderBox::showBar() const {
	return barRect().overlap(mousePos()) || draggingSlider;
}

inline float ReaderBox::getZoom() const {
	return zoom;
}
