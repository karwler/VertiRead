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

	void drawSelf() const override;
	void onResize() override;
	void tick(float dSec) override;
	void postInit() override;

	void onMouseMove(ivec2 mPos, ivec2 mMov) override;
	void onNavSelect(Direction) override {}
	bool navSelectable() const override;

	virtual void navSelectNext(sizet id, int mid, Direction dir);
	virtual void navSelectFrom(int mid, Direction dir);

	Widget* getWidget(sizet id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);	// not suitable for using on a ReaderBox, use the overload
	void replaceWidget(sizet id, Widget* widget);
	void deleteWidget(sizet id);
	const uset<Widget*>& getSelected() const;
	virtual ivec2 wgtPosition(sizet id) const;
	virtual ivec2 wgtSize(sizet id) const;
	void selectWidget(sizet id);
	void deselectWidget(sizet id);
	int getSpacing() const;

protected:
	void initWidgets(vector<Widget*>&& wgts);
	void clearWidgets();
	virtual ivec2 listSize() const;

	void navSelectWidget(sizet id, int mid, Direction dir);
private:
	void scanSequential(sizet id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);

	mvec2 findMinMaxSelectedID() const;
	static bool anyWidgetsNavSelected(Layout* box);
};

inline Widget* Layout::getWidget(sizet id) const {
	return widgets[id];
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline const uset<Widget*>& Layout::getSelected() const {
	return selected;
}

inline void Layout::deselectWidget(sizet id) {
	selected.erase(widgets[id]);
}

inline int Layout::getSpacing() const {
	return spacing;
}

// top level layout
class RootLayout : public Layout {
public:
	using Layout::Layout;
	~RootLayout() override = default;

	ivec2 position() const override;
	ivec2 size() const override;
	Rect frame() const override;
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public RootLayout {
public:
	Color bgColor;
	Widget* firstNavSelect;
protected:
	Size sizeY;	// use Widget's relSize as width

public:
	Popup(const svec2& size = svec2(0), vector<Widget*>&& children = vector<Widget*>(), Widget* first = nullptr, Color background = Color::normal, Direction dir = defaultDirection, int space = defaultItemSpacing, bool pad = true);
	~Popup() override = default;

	void drawSelf() const override;

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
	Rect actRect() const;
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
	void setRect(const Rect& rct);
};

template <class T>
inline T* Context::owner() const {
	return reinterpret_cast<T*>(parent);
}

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
protected:
	bool draggingSlider = false;
	ivec2 listPos{ 0, 0 };
private:
	vec2 motion = { 0.f, 0.f };	// how much the list scrolls over time
	int diffSliderMouse = 0;	// space between slider and mouse position

	static constexpr float scrollThrottle = 10.f;

public:
	using Layout::Layout;
	~ScrollArea() override = default;

	void drawSelf() const override;
	void onResize() override;
	void tick(float dSec) override;
	void postInit() override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(uint8 mBut) override;
	void onScroll(ivec2 wMov) override;

	void navSelectNext(sizet id, int mid, Direction dir) override;
	void navSelectFrom(int mid, Direction dir) override;
	void scrollToWidgetPos(sizet id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(sizet id);
	bool scrollToNext();				// scroll to next widget (returns false if at scroll limit)
	bool scrollToPrevious();			// scroll to previous widget (returns false if at scroll limit)
	void scrollToLimit(bool start);		// scroll to start or end of the list relative to it's direction

	Rect frame() const override;
	ivec2 wgtPosition(sizet id) const override;
	ivec2 wgtSize(sizet id) const override;
	Rect barRect() const;
	Rect sliderRect() const;
	mvec2 visibleWidgets() const;
	void moveListPos(ivec2 mov);

protected:
	void scrollToSelected();
	virtual ivec2 listLim() const;	// max list position
	virtual int wgtRPos(sizet id) const;
	virtual int wgtREnd(sizet id) const;

private:
	void scrollToFollowing(sizet id, bool prev);
	void setSlider(int spos);
	int barSize() const;	// returns 0 if slider isn't needed
	int sliderSize() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position
	static void throttleMotion(float& mov, float dSec);
};

inline void ScrollArea::moveListPos(ivec2 mov) {
	listPos = clamp(listPos + mov, ivec2(0), listLim());
}

inline int ScrollArea::sliderLim() const {
	return size()[direction.vertical()] - sliderSize();
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

	void onResize() final;

	void navSelectNext(sizet id, int mid, Direction dir) final;
	void navSelectFrom(int mid, Direction dir) final;

	ivec2 wgtSize(sizet id) const final;

private:
	void scanVertically(sizet id, int mid, Direction dir);
	void scanHorizontally(sizet id, int mid, Direction dir);
	void scanFromStart(int mid, Direction dir);
	void scanFromEnd(int mid, Direction dir);
	void navSelectIfInRange(sizet id, int mid, Direction dir);

	int wgtREnd(sizet id) const final;
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
private:
	static constexpr float menuHideTimeout = 3.f;
	static inline const string emptyFile;

	vector<Texture> pics;
	float cursorTimer = menuHideTimeout;	// time left until cursor/overlay disappears
	float zoom;
	bool countDown = true;	// whether to decrease cursorTimer until cursor hide

public:
	ReaderBox(const Size& size = Size(), vector<Texture>&& imgs = vector<Texture>(), Direction dir = defaultDirection, float fzoom = Settings::defaultZoom, int space = Settings::defaultSpacing, bool pad = false);
	~ReaderBox() final;

	void drawSelf() const final;
	void onResize() final;
	void tick(float dSec) final;
	void postInit() final;
	void onMouseMove(ivec2 mPos, ivec2 mMov) final;

	void setWidgets(vector<Texture>&& imgs);
	bool showBar() const;
	float getZoom() const;
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the center
	const string& firstPage() const;
	const string& lastPage() const;
	const string& curPage() const;
	ivec2 wgtPosition(sizet id) const final;
	ivec2 wgtSize(sizet id) const final;

private:
	void initWidgets(vector<Texture>&& imgs);
	ivec2 listSize() const final;
	int wgtRPos(sizet id) const final;
	int wgtREnd(sizet id) const final;
};

inline bool ReaderBox::showBar() const {
	return barRect().contain(mousePos()) || draggingSlider;
}

inline float ReaderBox::getZoom() const {
	return zoom;
}

inline const string& ReaderBox::firstPage() const {
	return !pics.empty() ? pics.front().name : emptyFile;
}

inline const string& ReaderBox::lastPage() const {
	return !pics.empty() ? pics.back().name : emptyFile;
}

inline const string& ReaderBox::curPage() const {
	return !pics.empty() ? pics[direction.positive() ? visibleWidgets().x : visibleWidgets().y - 1].name : emptyFile;
}
