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

	static constexpr int defaultItemSpacing = 5;
	static constexpr Direction::Dir defaultDirection = Direction::down;

protected:
	vector<Widget*> widgets;
	vector<ivec2> positions;	// widgets' positions. one element larger than widgets. last element is layout's size
	uset<Widget*> selected;
	int spacing;				// space between widgets
	bool margin;				// use spacing around the widgets as well
	Select selection;			// how many items can be selected at once
	Direction direction;		// in what direction widgets are stacked (TileBox doesn't support horizontal)

public:
	Layout(const Size& size = Size(), vector<Widget*>&& children = vector<Widget*>(), Direction dir = defaultDirection, Select select = Select::none, int space = defaultItemSpacing, bool pad = false);
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

	virtual void navSelectNext(size_t id, int mid, Direction dir);
	virtual void navSelectFrom(int mid, Direction dir);

	template <class T = Widget> T* getWidget(size_t id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);	// not suitable for using on a ReaderBox, use the overload
	void insertWidget(size_t id, Widget* wgt);
	void replaceWidget(size_t id, Widget* widget);
	void deleteWidget(size_t id);
	const uset<Widget*>& getSelected() const;
	virtual ivec2 wgtPosition(size_t id) const;
	virtual ivec2 wgtSize(size_t id) const;
	void selectWidget(size_t id);
	void deselectWidget(size_t id);
	int getSpacing() const;
	bool isVertical() const;

protected:
	void initWidgets(vector<Widget*>&& wgts);
	void clearWidgets();
	virtual void calculateWidgetPositions();
	virtual void postWidgetsChange();
	virtual ivec2 listSize() const;

	void navSelectWidget(size_t id, int mid, Direction dir);
private:
	void scanSequential(size_t id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);
	mvec2 findMinMaxSelectedID() const;
};

template <class T>
T* Layout::getWidget(size_t id) const {
	return static_cast<T*>(widgets[id]);
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline const uset<Widget*>& Layout::getSelected() const {
	return selected;
}

inline void Layout::deselectWidget(size_t id) {
	selected.erase(widgets[id]);
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
class Context : public Popup {
private:
	svec2 pos;
	LCall resizeCall;

public:
	Context(const svec2& position = svec2(0), const svec2& size = svec2(0), vector<Widget*>&& children = vector<Widget*>(), Widget* first = nullptr, Widget* owner = nullptr, Color background = Color::dark, LCall resize = nullptr, Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = true);
	~Context() final = default;

	void onResize() final;
	ivec2 position() const final;

	template <class T = Widget> T* owner() const;
	void setRect(const Recti& rct);
};

template <class T>
inline T* Context::owner() const {
	return reinterpret_cast<T*>(parent);
}

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
protected:
	ScrollBar scroll;

public:
	using Layout::Layout;
	~ScrollArea() override = default;

	void drawSelf(const Recti& view) override;
	void onResize() override;
	void tick(float dSec) override;
	void postInit() override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;
	void onScroll(ivec2 wMov) override;

	void navSelectNext(size_t id, int mid, Direction dir) override;
	void navSelectFrom(int mid, Direction dir) override;
	void scrollToWidgetPos(size_t id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(size_t id);
	bool scrollToNext();				// scroll to next widget (returns false if at scroll limit)
	bool scrollToPrevious();			// scroll to previous widget (returns false if at scroll limit)
	void scrollToLimit(bool start);		// scroll to start or end of the list relative to it's direction
	float getScrollLocation() const;
	void setScrollLocation(float loc);

	Recti frame() const override;
	ivec2 wgtPosition(size_t id) const override;
	ivec2 wgtSize(size_t id) const override;
	mvec2 visibleWidgets() const;
	Recti barRect() const;
	Recti sliderRect() const;

protected:
	void scrollToSelected();
	void postWidgetsChange() override;
	virtual int wgtRPos(size_t id) const;
	virtual int wgtREnd(size_t id) const;

private:
	void scrollToFollowing(size_t id, bool prev);
};

inline Recti ScrollArea::barRect() const {
	return ScrollBar::barRect(listSize(), position(), size(), direction.vertical());
}

inline Recti ScrollArea::sliderRect() const {
	return scroll.sliderRect(listSize(), position(), size(), direction.vertical());
}

// places items as tiles one after another
class TileBox : public ScrollArea {
public:
	static constexpr int defaultItemHeight = 30;
private:
	int wheight;

public:
	TileBox(const Size& size = Size(), vector<Widget*>&& children = vector<Widget*>(), int childHeight = defaultItemHeight, Direction dir = defaultDirection, Select select = Select::none, int space = defaultItemSpacing, bool pad = false);
	~TileBox() final = default;

	void navSelectNext(size_t id, int mid, Direction dir) final;
	void navSelectFrom(int mid, Direction dir) final;

	ivec2 wgtSize(size_t id) const final;
protected:
	void calculateWidgetPositions() final;

private:
	void scanVertically(size_t id, int mid, Direction dir);
	void scanHorizontally(size_t id, int mid, Direction dir);
	void scanFromStart(int mid, Direction dir);
	void scanFromEnd(int mid, Direction dir);
	void navSelectIfInRange(size_t id, int mid, Direction dir);

	int wgtREnd(size_t id) const final;
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
private:
	static constexpr float menuHideTimeout = 3.f;
	static inline const string emptyFile;

	vector<string> picNames;
	size_t startPicId;
	float cursorTimer = menuHideTimeout;	// time left until cursor/overlay disappears
	float zoom;
	bool countDown = true;	// whether to decrease cursorTimer until cursor hide

public:
	ReaderBox(const Size& size = Size(), Direction dir = defaultDirection, float fzoom = Settings::defaultZoom, int space = Settings::defaultSpacing, bool pad = false);
	~ReaderBox() final = default;

	void drawSelf(const Recti& view) final;
	void tick(float dSec) final;
	void postInit() final;
	void onMouseMove(ivec2 mPos, ivec2 mMov) final;

	void setWidgets(vector<pair<string, Texture*>>& imgs, const string& startPic);
	bool showBar() const;
	float getZoom() const;
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the center
	const string& firstPage() const;
	const string& lastPage() const;
	const string& curPage() const;
	ivec2 wgtPosition(size_t id) const final;
	ivec2 wgtSize(size_t id) const final;

private:
	void calculateWidgetPositions() final;
	ivec2 listSize() const final;
	int wgtRPos(size_t id) const final;
	int wgtREnd(size_t id) const final;
};

inline float ReaderBox::getZoom() const {
	return zoom;
}

inline const string& ReaderBox::firstPage() const {
	return !picNames.empty() ? picNames.front() : emptyFile;
}

inline const string& ReaderBox::lastPage() const {
	return !picNames.empty() ? picNames.back() : emptyFile;
}

inline const string& ReaderBox::curPage() const {
	return !picNames.empty() ? picNames[direction.positive() ? visibleWidgets().x : visibleWidgets().y - 1] : emptyFile;
}
