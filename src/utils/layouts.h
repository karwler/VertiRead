#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
public:
	static constexpr int defaultItemSpacing = 5;
	static constexpr Direction::Dir defaultDirection = Direction::down;

protected:
	vector<Widget*> widgets;
	uptr<ivec2[]> positions;	// widgets' positions. one element larger than widgets. last element is layout's size
	int spacing;				// space between widgets
	bool margin;				// use spacing around the widgets as well
	Direction direction;		// in what direction widgets are stacked (TileBox doesn't support horizontal)

public:
	Layout(const Size& size = Size(), vector<Widget*>&& children = vector<Widget*>(), Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = false);
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

	template <Class T = Widget> T* getWidget(uint id) const;
	const vector<Widget*>& getWidgets() const;
	virtual void setWidgets(vector<Widget*>&& wgts);	// not suitable for using on a ReaderBox, use setPictures
	void insertWidget(uint id, Widget* wgt);
	void replaceWidget(uint id, Widget* widget);
	void deleteWidget(uint id);
	virtual ivec2 wgtPosition(uint id) const;
	virtual ivec2 wgtSize(uint id) const;
	int getSpacing() const;
	bool isVertical() const;

protected:
	void initWidgets(vector<Widget*>&& wgts);
	void clearWidgets();
	virtual void calculateWidgetPositions();
	virtual void postWidgetsChange();

	void navSelectWidget(uint id, int mid, Direction dir);
private:
	void scanSequential(uint id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);
};

template <Class T>
T* Layout::getWidget(uint id) const {
	return static_cast<T*>(widgets[id]);
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline int Layout::getSpacing() const {
	return spacing;
}

inline bool Layout::isVertical() const {
	return direction.vertical();
}

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
	Color bgColor;
	Widget* firstNavSelect;
	PCall ccall;	// gets called on escape press
protected:
	Size sizeY;		// use Widget's relSize as width

public:
	Popup(const svec2& size = svec2(0), vector<Widget*>&& children = vector<Widget*>(), PCall cancelCall = nullptr, Widget* first = nullptr, Color background = Color::normal, Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = true);
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
	svec2 pos;
	svec2 actPos, actSize;	// positions and size of activation area

public:
	Overlay(const svec2& position = svec2(0), const svec2& size = svec2(0), const svec2& activationPos = svec2(0), const svec2& activationSize = svec2(0), vector<Widget*>&& children = vector<Widget*>(), Color background = Color::normal, Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = false);
	~Overlay() override = default;

	ivec2 position() const override;
	Recti actRect() const;
};

// mix between popup and overlay for context menus
class Context final : public Popup {
private:
	svec2 pos;
	LCall resizeCall;

public:
	Context(const svec2& position = svec2(0), const svec2& size = svec2(0), vector<Widget*>&& children = vector<Widget*>(), Widget* first = nullptr, Widget* owner = nullptr, Color background = Color::dark, LCall resize = nullptr, Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = true);
	~Context() final = default;

	void onResize() final;
	ivec2 position() const final;

	template <Class T = Widget> T* owner() const;
	void setRect(const Recti& rct);
};

template <Class T>
inline T* Context::owner() const {
	return reinterpret_cast<T*>(parent);
}

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout, protected Scrollable {
public:
	enum class Select : uint8 {
		none,
		one,
		any
	};

private:
	uset<Widget*> selected;
	Select selection;	// how many items can be selected at once

public:
	ScrollArea(const Size& size = Size(), vector<Widget*>&& children = vector<Widget*>(), Direction dir = defaultDirection, Select select = Select::none, int space = defaultItemSpacing, bool pad = false);
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

	void setWidgets(vector<Widget*>&& wgts) override;
	Recti frame() const override;
	ivec2 wgtPosition(uint id) const override;
	ivec2 wgtSize(uint id) const override;
	uvec2 visibleWidgets() const;
	Recti barRect() const;
	Recti sliderRect() const;
	const uset<Widget*>& getSelected() const;
	void selectWidget(uint id);
	void deselectWidget(uint id);

protected:
	void scrollToSelected();
	void calculateWidgetPositions() override;
	void postWidgetsChange() override;
	virtual int wgtRPos(uint id) const;
	virtual int wgtREnd(uint id) const;
	uint firstWidgetAt(int rpos) const;

private:
	void scrollToFollowing(uint id, bool prev);
	uvec2 findMinMaxSelectedID() const;
};

inline Recti ScrollArea::barRect() const {
	return Scrollable::barRect(position(), size(), direction.vertical());
}

inline Recti ScrollArea::sliderRect() const {
	return Scrollable::sliderRect(position(), size(), direction.vertical());
}

inline const uset<Widget*>& ScrollArea::getSelected() const {
	return selected;
}

inline void ScrollArea::deselectWidget(uint id) {
	selected.erase(widgets[id]);
}

// places items as tiles one after another
class TileBox final : public ScrollArea {
public:
	static constexpr int defaultItemHeight = 30;

private:
	int wheight;

public:
	TileBox(const Size& size = Size(), vector<Widget*>&& children = vector<Widget*>(), int childHeight = defaultItemHeight, Direction dir = defaultDirection, Select select = Select::none, int space = defaultItemSpacing, bool pad = false);
	~TileBox() final = default;

	void navSelectNext(uint id, int mid, Direction dir) final;
	void navSelectFrom(int mid, Direction dir) final;

	ivec2 wgtSize(uint id) const final;
protected:
	void calculateWidgetPositions() final;

private:
	void scanVertically(uint id, int mid, Direction dir);
	void scanHorizontally(uint id, int mid, Direction dir);
	void scanFromStart(int mid, Direction dir);
	void scanFromEnd(int mid, Direction dir);
	void navSelectIfInRange(uint id, int mid, Direction dir);

	int wgtREnd(uint id) const final;
};

// for scrolling through pictures
class ReaderBox final : public ScrollArea {
private:
	static constexpr float menuHideTimeout = 3.f;

	bool countDown = true;	// whether to decrease cursorTimer until cursor hide
	uptr<string[]> picNames;
	uint startPicId;
	float cursorTimer = menuHideTimeout;	// time left until cursor/overlay disappears
	float zoom;

public:
	ReaderBox(const Size& size = Size(), Direction dir = defaultDirection, float fzoom = Settings::defaultZoom, int space = Settings::defaultSpacing, bool pad = false);
	~ReaderBox() final = default;

	void drawSelf(const Recti& view) final;
	void tick(float dSec) final;
	void onMouseMove(ivec2 mPos, ivec2 mMov) final;

	void setPictures(vector<pair<string, Texture*>>& imgs, string_view startPic);
	bool showBar() const;
	float getZoom() const;
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the center
	string_view firstPage() const;
	string_view lastPage() const;
	string_view curPage() const;
	ivec2 wgtSize(uint id) const final;

private:
	void calculateWidgetPositions() final;
};

inline float ReaderBox::getZoom() const {
	return zoom;
}

inline string_view ReaderBox::firstPage() const {
	return !widgets.empty() ? picNames[0] : string_view();
}

inline string_view ReaderBox::lastPage() const {
	return !widgets.empty() ? picNames[widgets.size() - 1] : string_view();
}

inline string_view ReaderBox::curPage() const {
	return !widgets.empty() ? picNames[direction.positive() ? firstWidgetAt(listPos[direction.vertical()]) : visibleWidgets().y - 1] : string_view();
}
