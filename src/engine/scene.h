#pragma once

#include "utils/layouts.h"

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	ClickStamp(Widget* WGT=nullptr, ScrollArea* ARE=nullptr, const vec2i& POS=0);

	Widget* widget;
	ScrollArea* area;
	vec2i mPos;
};

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Scene();
	~Scene();

	void tick(float dSec);
	void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	void onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(const vec2i& mPos, uint8 mBut);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();
	void onText(const char* str);
	void onResize();

	void resetLayouts();
	Layout* getLayout() { return layout.get(); }
	vector<Overlay*>& getOverlays() { return overlays; }
	Popup* getPopup() { return popup.get(); }
	void setPopup(Popup* newPopup, Widget* newCapture=nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap) { setPopup(popcap.first, popcap.second); }

	bool cursorInClickRange(const vec2i& mPos, uint8 mBut);
	ScrollArea* getFocusedScrollArea() const;

	Widget* capture;				// either pointer to widget currently hogging all keyboard input or ScrollArea whichs slider is currently being dragged. nullptr if nothing is being captured or dragged
private:
	uptr<Layout> layout;	// main layout
	uptr<Popup> popup;
	vector<Overlay*> overlays;

	vector<Widget*> focused;	// list of widgets over which the cursor is poistioned
	vector<ClickStamp> stamps;	// data about last mouse click (indexes are mouse button numbers)

	void setFocused(const vec2i& mPos);
	void setFocusedElement(const vec2i& mPos, Layout* box);
	Overlay* findFocusedOverlay(const vec2i& mPos);
};
