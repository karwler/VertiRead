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
protected:
	static constexpr Direction::Dir defaultDirection = Direction::down;

	vector<Widget*> widgets;
	vector<vec2i> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	uset<Widget*> selected;
	int spacing;				// space between widgets
	Select selection;			// how many items can be selected at once
	Direction direction;		// in what direction widgets are stacked (TileBox doesn't support horizontal)

public:
	Layout(const Size& relSize = Size(), vector<Widget*>&& children = {}, Direction direction = defaultDirection, Select select = Select::none, int spacing = defaultItemSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Layout() override;

	virtual void drawSelf() const override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;

	virtual void onMouseMove(vec2i mPos, vec2i mMov) override;
	virtual void onNavSelect(Direction) override {}
	virtual bool navSelectable() const override;

	virtual void navSelectNext(sizet id, int mid, Direction dir);
	virtual void navSelectFrom(int mid, Direction dir);

	Widget* getWidget(sizet id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);	// not suitable for using on a ReaderBox, use the overload
	void replaceWidget(sizet id, Widget* widget);
	void deleteWidget(sizet id);
	const uset<Widget*> getSelected() const;
	virtual vec2i wgtPosition(sizet id) const;
	virtual vec2i wgtSize(sizet id) const;
	void selectWidget(sizet id);
	void deselectWidget(sizet id);

protected:
	void initWidgets(vector<Widget*>&& wgts);
	virtual vec2i listSize() const;

	void navSelectWidget(sizet id, int mid, Direction dir);
private:
	void scanSequential(sizet id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);

	vec2t findMinMaxSelectedID() const;
};

inline Widget* Layout::getWidget(sizet id) const {
	return widgets[id];
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline const uset<Widget*> Layout::getSelected() const {
	return selected;
}

inline void Layout::deselectWidget(sizet id) {
	selected.erase(widgets[id]);
}

// top level layout
class RootLayout : public Layout {
public:
	RootLayout(const Size& relSize = Size(), vector<Widget*>&& children = {}, Direction direction = defaultDirection, Select select = Select::none, int spacing = defaultItemSpacing);
	virtual ~RootLayout() override = default;

	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual Rect frame() const override;
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public RootLayout {
private:
	Size sizeY;	// use Widget's relSize as sizeX

public:
	Popup(const vec2s& relSize = vec2s(0), vector<Widget*>&& children = {}, Direction direction = defaultDirection, int spacing = defaultItemSpacing);
	virtual ~Popup() override = default;

	virtual void drawSelf() const override;

	virtual vec2i position() const override;
	virtual vec2i size() const override;
};

// popup that can be enabled or disabled
class Overlay : public Popup {
public:
	bool on;
private:
	vec2s pos;
	vec2s actPos, actSize;	// positions and size of activation area

public:
	Overlay(const vec2s& position = vec2s(0), const vec2s& relSize = vec2s(0), const vec2s& activationPos = vec2s(0), const vec2s& activationSize = vec2s(0), vector<Widget*>&& children = {}, Direction direction = Direction::down, int spacing = defaultItemSpacing);
	virtual ~Overlay() override = default;

	virtual vec2i position() const override;
	Rect actRect() const;
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
protected:
	bool draggingSlider;
	vec2i listPos;
private:
	vec2f motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position

	static constexpr float scrollThrottle = 10.f;

public:
	ScrollArea(const Size& relSize = Size(), vector<Widget*>&& children = {}, Direction direction = defaultDirection, Select select = Select::none, int spacing = defaultItemSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~ScrollArea() override = default;

	virtual void drawSelf() const override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(vec2i mPos, uint8 mBut) override;
	virtual void onDrag(vec2i mPos, vec2i mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(vec2i wMov) override;

	virtual void navSelectNext(sizet id, int mid, Direction dir) override;
	virtual void navSelectFrom(int mid, Direction dir) override;
	void scrollToWidgetPos(sizet id);	// set listPos.y to the widget's position
	void scrollToWidgetEnd(sizet id);
	bool scrollToNext();				// scroll to next widget (returns false if at scroll limit)
	bool scrollToPrevious();			// scroll to previous widget (returns false if at scroll limit)
	void scrollToLimit(bool start);		// scroll to start or end of the list relative to it's direction

	virtual Rect frame() const override;
	virtual vec2i wgtPosition(sizet id) const override;
	virtual vec2i wgtSize(sizet id) const override;
	Rect barRect() const;
	Rect sliderRect() const;
	vec2t visibleWidgets() const;
	void moveListPos(vec2i mov);

protected:
	void scrollToSelected();
	virtual vec2i listLim() const;	// max list position
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

inline void ScrollArea::moveListPos(vec2i mov) {
	listPos = (listPos + mov).clamp(0, listLim());
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
	TileBox(const Size& relSize = Size(), vector<Widget*>&& children = {}, int childHeight = defaultItemHeight, Direction direction = defaultDirection, Select select = Select::none, int spacing = defaultItemSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~TileBox() override = default;

	virtual void onResize() override;

	virtual void navSelectNext(sizet id, int mid, Direction dir) override;
	virtual void navSelectFrom(int mid, Direction dir) override;

	virtual vec2i wgtSize(sizet id) const override;

private:
	void scanVertically(sizet id, int mid, Direction dir);
	void scanHorizontally(sizet id, int mid, Direction dir);
	void scanFromStart(int mid, Direction dir);
	void scanFromEnd(int mid, Direction dir);
	void navSelectIfInRange(sizet id, int mid, Direction dir);

	virtual int wgtREnd(sizet id) const override;
};

// for scrolling through pictures
class ReaderBox : public ScrollArea {
private:
	static constexpr float menuHideTimeout = 3.f;
	static inline const string emptyFile;

	vector<Texture> pics;
	float cursorTimer;		// time left until cursor/overlay disappeares
	float zoom;
	bool countDown;			// whether to decrease cursorTimer until cursor hide

public:
	ReaderBox(const Size& relSize = Size(), vector<Texture>&& imgs = {}, Direction direction = defaultDirection, float zoom = Settings::defaultZoom, int spacing = Settings::defaultSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~ReaderBox() override;

	virtual void drawSelf() const override;
	virtual void onResize() override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onMouseMove(vec2i mPos, vec2i mMov) override;

	void setWidgets(vector<Texture>&& imgs);
	bool showBar() const;
	float getZoom() const;
	void setZoom(float factor);
	void centerList();		// set listPos.x so that the view will be in the centter
	const string& firstPage() const;
	const string& lastPage() const;
	const string& curPage() const;
	virtual vec2i wgtPosition(sizet id) const override;
	virtual vec2i wgtSize(sizet id) const override;

private:
	void initWidgets(vector<Texture>&& imgs);
	virtual vec2i listSize() const override;
	virtual int wgtRPos(sizet id) const override;
	virtual int wgtREnd(sizet id) const override;
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
	return !pics.empty() ? pics[direction.positive() ? visibleWidgets().b : visibleWidgets().t - 1].name : emptyFile;
}
