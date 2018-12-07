#pragma once

#include "utils/layouts.h"

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	ClickStamp(Widget* widget = nullptr, ScrollArea* area = nullptr, const vec2i& mPos = 0);

	Widget* widget;
	ScrollArea* area;
	vec2i mPos;
};

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Scene();

	void tick(float dSec);
	void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	void onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(const vec2i& mPos, uint8 mBut);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();
	void onText(const string& str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	void onResize();

	void resetLayouts();
	Layout* getLayout();
	Overlay* getOverlay();
	Popup* getPopup();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);

	void selectFirst();
	sizt findSelectedID(Layout* box);	// get id of possibly select or select's parent in relation to box
	bool cursorDisableable();
	bool cursorInClickRange(const vec2i& mPos, uint8 mBut);

	Widget* select;		// currently selected widget
	Widget* capture;	// either pointer to widget currently hogging all keyboard input or ScrollArea whichs slider is currently being dragged. nullptr if nothing is being captured or dragged
private:
	uptr<Layout> layout;
	uptr<Popup> popup;
	uptr<Overlay> overlay;
	vector<ClickStamp> stamps;	// data about last mouse click (indexes are mouse button numbers

	void setSelected(const vec2i& mPos, Layout* box);
	ScrollArea* getSelectedScrollArea() const;
	bool overlayFocused(const vec2i& mPos);
	Layout* topLayout(const vec2i& mPos);
};

inline void Scene::onText(const string& str) {
	capture->onText(str);
}

inline Layout* Scene::getLayout() {
	return layout.get();
}

inline Overlay* Scene::getOverlay() {
	return overlay.get();
}

inline Popup* Scene::getPopup() {
	return popup.get();
}

inline void Scene::setPopup(const pair<Popup*, Widget*>& popcap) {
	setPopup(popcap.first, popcap.second);
}

inline bool Scene::cursorInClickRange(const vec2i& mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= Default::clickThreshold;
}

inline Layout* Scene::topLayout(const vec2i& mPos) {
	return popup ? popup.get() : overlayFocused(mPos) ? overlay.get() : layout.get();
}
