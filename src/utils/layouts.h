#pragma once

#include "widgets.h"
#include <span>

struct Children {
	uptr<Widget*[]> wgts;
	uint num = 0;

	Children() = default;
	Children(Children&&) = default;
	Children(std::initializer_list<Widget*> ilist);
	template <Class T> Children(T&& view);
	Children(uint size);

	Children& operator=(Children&& ch);
	Widget*& operator[](uint id) { return wgts[id]; }
	Widget* const& operator[](uint id) const { return wgts[id]; }
};

template <Class T>
Children::Children(T&& view) :
	wgts(std::make_unique_for_overwrite<Widget*[]>(view.size())),
	num(view.size())
{
	rng::copy(view, wgts.get());
}

// container for other widgets
class Layout : public Widget {
public:
	static constexpr ushort defaultItemSpacing = 5;
	static constexpr Direction::Dir defaultDirection = Direction::down;

protected:
	uptr<Widget*[]> widgets;
	uptr<ivec2[]> positions;	// widgets' positions. one element larger than widgets. last element is layout's size
	uint numWgts = 0;
	const ushort spacing;		// space between widgets
	const bool margin;			// use spacing around the widgets as well
public:
	const Direction direction;	// in what direction widgets are stacked (TileBox doesn't support horizontal)

	Layout(const Size& size, Children&& children, Direction dir = defaultDirection, ushort space = defaultItemSpacing, bool pad = false);
	~Layout() override;

	void drawSelf(const Recti& view) override;
	void drawAddr(const Recti& view) override;
	void onResize() override;
	void tick(float dSec) override;
	void postInit() override;
	void onMouseMove(ivec2 mPos, ivec2 mMov) override;
	void onDisplayChange() override;
	void onNavSelect(Direction) override {}
	bool navSelectable() const override;

	virtual void navSelectNext(uint id, int mid, Direction dir);
	virtual void navSelectFrom(int mid, Direction dir);

	template <Class T = Widget> T* getWidget(uint id) const { return static_cast<T*>(widgets[id]); }
	std::span<Widget*> getWidgets() const { return std::span(widgets.get(), numWgts); }
	virtual void setWidgets(Children&& children);	// not suitable for using on a ReaderBox, use setPictures
	void insertWidget(uint id, Widget* wgt);
	void replaceWidget(uint id, Widget* widget);
	void deleteWidget(uint id);
	virtual ivec2 wgtPosition(uint id) const;
	virtual ivec2 wgtSize(uint id) const;
	int getSpacing() const { return spacing; }
	bool isVertical() const { return direction.vertical(); }

protected:
	void initWidgets(Children&& children);
	void clearWidgets();
	virtual void calculateWidgetPositions();
	virtual void postWidgetsChange();

	void navSelectWidget(uint id, int mid, Direction dir);
	void deselectWidgets() const;
private:
	void deselectWidget(const Widget* wgt) const;
	void scanSequential(uint id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);
	bool isParentOf(const Widget* wgt) const;
};

// top level layout
class RootLayout : public Layout {
public:
	using Layout::Layout;
	~RootLayout() override = default;

	ivec2 position() const override;
	ivec2 size() const override;
	Recti frame() const override;
	void setSize(const Size& size) override;
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public RootLayout {
public:
	const Color bgColor;
	Widget* const firstNavSelect;
	const EventId ccall;	// gets called on escape press
protected:
	Size sizeY;		// use Widget's relSize as width

public:
	Popup(const svec2& size, Children&& children, EventId cancelCall = nullEvent, Widget* first = nullptr, Color background = Color::normal, Direction dir = defaultDirection, ushort space = defaultItemSpacing, bool pad = true);
	~Popup() override = default;

	void drawSelf(const Recti& view) override;

	ivec2 position() const override;
	ivec2 size() const override;
};

// popup that can be enabled or disabled
class Overlay : public Popup {
public:
	bool on = false;
private:
	const svec2 pos;
	const svec2 actPos, actSize;	// positions and size of activation area

public:
	Overlay(const svec2& position, const svec2& size, const svec2& activationPos, const svec2& activationSize, Children&& children, Color background = Color::normal, Direction dir = defaultDirection, ushort space = defaultItemSpacing, bool pad = false);
	~Overlay() override = default;

	ivec2 position() const override;
	Recti actRect() const;
};

// mix between popup and overlay for context menus
class Context final : public Popup {
private:
	svec2 pos;
	const EventId resizeCall;

public:
	Context(const svec2& position, const svec2& size, Children&& children, Widget* first = nullptr, Widget* owner = nullptr, Color background = Color::dark, EventId resize = nullEvent, Direction dir = defaultDirection, ushort space = defaultItemSpacing, bool pad = true);
	~Context() override = default;

	void onResize() override;
	ivec2 position() const override;

	template <Class T = Widget> T* owner() const { return reinterpret_cast<T*>(parent); }
	void setRect(const Recti& rct);
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout, protected Scrollable {
public:
	using Layout::Layout;
	~ScrollArea() override = default;

	void drawSelf(const Recti& view) override;
	void onResize() override;
	void tick(float dSec) override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;
	void onScroll(ivec2 wMov) override;

	void navSelectNext(uint id, int mid, Direction dir) override;
	void navSelectFrom(int mid, Direction dir) override;
	void scrollToWidgetPos(uint id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(uint id);
	bool scrollToNext();				// scroll to next widget (returns false if at scroll limit)
	bool scrollToPrevious();			// scroll to previous widget (returns false if at scroll limit)
	void scrollToLimit(bool start);		// scroll to start or end of the list relative to it's direction
	float getScrollLocation() const;
	void setScrollLocation(float loc);

	void setWidgets(Children&& children) override;
	Recti frame() const override;
	ivec2 wgtPosition(uint id) const override;
	ivec2 wgtSize(uint id) const override;
	uvec2 visibleWidgets() const;
	Recti barRect() const;
	Recti sliderRect() const;

protected:
	void scrollToSelected();
	void calculateWidgetPositions() override;
	void postWidgetsChange() override;
	virtual int wgtRPos(uint id) const;
	virtual int wgtREnd(uint id) const;
	uint firstWidgetAt(int rpos) const;

private:
	void scrollToFollowing(uint id, bool prev);
};

inline Recti ScrollArea::barRect() const {
	return Scrollable::barRect(position(), size(), direction.vertical());
}

inline Recti ScrollArea::sliderRect() const {
	return Scrollable::sliderRect(position(), size(), direction.vertical());
}

// places items as tiles one after another
class TileBox final : public ScrollArea {
public:
	static constexpr int defaultItemHeight = 30;

private:
	const int wheight;

public:
	TileBox(const Size& size, Children&& children, int childHeight = defaultItemHeight, Direction dir = defaultDirection, ushort space = defaultItemSpacing, bool pad = false);
	~TileBox() override = default;

	void navSelectNext(uint id, int mid, Direction dir) override;
	void navSelectFrom(int mid, Direction dir) override;

	ivec2 wgtSize(uint id) const override;
protected:
	void calculateWidgetPositions() override;

private:
	void scanVertically(uint id, int mid, Direction dir);
	void scanHorizontally(uint id, int mid, Direction dir);
	void scanFromStart(int mid, Direction dir);
	void scanFromEnd(int mid, Direction dir);
	void navSelectIfInRange(uint id, int mid, Direction dir);

	int wgtREnd(uint id) const override;
};

// for scrolling through pictures
class ReaderBox final : public ScrollArea {
private:
	static constexpr float menuHideTimeout = 3.f;

	uptr<string[]> picNames;
	uint startPicId;
	float cursorTimer = menuHideTimeout;	// time left until cursor/overlay disappears
	int8 zoomStep;
	bool countDown = true;	// whether to decrease cursorTimer until cursor hide

public:
	ReaderBox(const Size& size, Direction dir, int8 zstep, ushort space, bool pad = false);
	~ReaderBox() override = default;

	void drawSelf(const Recti& view) override;
	void tick(float dSec) override;
	void onMouseMove(ivec2 mPos, ivec2 mMov) override;

	void setPictures(vector<pair<string, Texture*>>& imgs, string_view startPic);
	bool showBar() const;
	void setZoom(int8 step);
	void addZoom(int8 step);
	void centerList();		// set listPos.x so that the view will be in the center
	string_view firstPage() const;
	string_view lastPage() const;
	string_view curPage() const;
	ivec2 wgtSize(uint id) const override;

private:
	void calculateWidgetPositions() override;
	template <Invocable<int8> F> void setZoom(F zset, int8 step);
};

inline string_view ReaderBox::firstPage() const {
	return numWgts ? picNames[0] : string_view();
}

inline string_view ReaderBox::lastPage() const {
	return numWgts ? picNames[numWgts - 1] : string_view();
}

inline string_view ReaderBox::curPage() const {
	return numWgts ? picNames[direction.positive() ? firstWidgetAt(listPos[direction.vertical()]) : visibleWidgets().y - 1] : string_view();
}
