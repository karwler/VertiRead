#pragma once

#include "utils/utils.h"
#include <SDL_video.h>
#include <glm/geometric.hpp>

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Widget* widget;
	ScrollArea* area;
	ivec2 mPos;

	ClickStamp(Widget* wgt = nullptr, ScrollArea* sarea = nullptr, ivec2 cursPos = ivec2(0)) noexcept;
};

// handles more back-end UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
private:
	Widget* select = nullptr;	// currently selected widget
	Widget* capture = nullptr;	// either pointer to widget currently hogging all keyboard input or ScrollArea which's slider is currently being dragged. nullptr if nothing is being captured or dragged
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_Window* captureWindow;	// window associated with currently captured widget
#endif
	RootLayout* layout = nullptr;
	Popup* popup = nullptr;
	Overlay* overlay = nullptr;
	Context* context = nullptr;
	array<ClickStamp, 3> stamps;	// data about last mouse click (indices are mouse button numbers
	uint captureLen = 0;	// composing substring length

	static constexpr float clickMoveThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;

public:
	~Scene();

	void tick(float dSec);
	void onMouseMove(ivec2 mPos, ivec2 mMov);
	void onMouseDown(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseWheel(ivec2 wMov);
	void onMouseLeave();
	void onCompose(string_view str);
	void onText(string_view str);
	void onConfirm();
	void onCancel();
	void onResize();
	void onDisplayChange();

	Widget* getSelect() const { return select; }
	Widget* getCapture() const { return capture; }
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_Window* getCaptureWindow() const { return captureWindow; }
#endif
	void setCapture(Widget* inter) noexcept;
	void resetLayouts();
	void clearLayouts();
	void setLayouts();
	RootLayout* getLayout() { return layout; }
	Overlay* getOverlay() { return overlay; }
	Popup* getPopup() { return popup; }
	Popup* releasePopup();	// can only be used for popups without capture or contexts
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	Context* getContext() { return context; }
	void setContext(Context* newContext);

	void updateSelect();
	void updateSelect(Widget* sel);
	void deselect();
	void selectFirst();
	bool cursorInClickRange(ivec2 mPos, uint8 mBut);

	ScrollArea* getSelectedScrollArea() const noexcept;
private:
	Widget* getSelected(ivec2 mPos);
	bool overlayFocused(ivec2 mPos) const;
	void finishNewPopup(Popup* lay);
};

inline bool Scene::cursorInClickRange(ivec2 mPos, uint8 mBut) {
	return glm::length(vec2(mPos - stamps[mBut - 1].mPos)) <= clickMoveThreshold;
}
