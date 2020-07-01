#pragma once

#include "utils/layouts.h"

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Widget* widget;
	ScrollArea* area;
	ivec2 mPos;

	ClickStamp(Widget* wgt = nullptr, ScrollArea* sarea = nullptr, ivec2 cursPos = ivec2(0));
};

// handles more back-end UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Widget* select;		// currently selected widget
	Widget* capture;	// either pointer to widget currently hogging all keyboard input or ScrollArea which's slider is currently being dragged. nullptr if nothing is being captured or dragged
private:
	uptr<RootLayout> layout;
	uptr<Popup> popup;
	uptr<Overlay> overlay;
	array<ClickStamp, SDL_BUTTON_RIGHT> stamps;	// data about last mouse click (indexes are mouse button numbers

	static constexpr float clickMoveThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;

public:
	Scene();

	void tick(float dSec);
	void onMouseMove(ivec2 mPos, ivec2 mMov);
	void onMouseDown(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseWheel(ivec2 wMov);
	void onMouseLeave();
	void onText(const char* str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
	void onResize();

	void resetLayouts();
	RootLayout* getLayout();
	Overlay* getOverlay();
	Popup* getPopup();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);

	void updateSelect();
	void updateSelect(ivec2 mPos);
	void selectFirst();
	sizet findSelectedID(Layout* box);	// get id of possibly select or selects parent in relation to box
	bool cursorDisableable();
	bool cursorInClickRange(ivec2 mPos, uint8 mBut);

private:
	Widget* getSelected(ivec2 mPos);
	ScrollArea* getSelectedScrollArea() const;
	bool overlayFocused(ivec2 mPos);
};

inline RootLayout* Scene::getLayout() {
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

inline void Scene::updateSelect(ivec2 mPos) {
	select = getSelected(mPos);
}

inline bool Scene::cursorInClickRange(ivec2 mPos, uint8 mBut) {
	return vec2(mPos - stamps[mBut-1].mPos).length() <= clickMoveThreshold;
}
