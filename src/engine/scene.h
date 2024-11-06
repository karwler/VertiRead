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
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_Window* captureWindow;	// window associated with currently captured widget
#endif
	RootLayout* layout = nullptr;
	Popup* popup = nullptr;
	Overlay* overlay = nullptr;
	Context* context = nullptr;
	array<ClickStamp, 3> stamps;	// data about last mouse click (indices are mouse button numbers
	uint captureLen = 0;	// composing substring length

	static constexpr float clickMoveThreshold = 8.f;

public:
	~Scene();

	void tick(float dSec);
	void onMouseMove(ivec2 mPos, ivec2 mMov);
	void onMouseDown(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(ivec2 mPos, uint8 mBut, uint8 mCnt);
	void onMouseWheel(vec2 wMov);
	void onMouseLeave() noexcept;
	void onCompose(string_view str);
	void onText(string_view str);
	void onConfirm();
	void onCancel();
	void onResize();
	void onDisplayChange();

	Widget* getSelect() const noexcept { return select; }
	Widget* getCapture() const noexcept { return capture; }
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_Window* getCaptureWindow() const noexcept { return captureWindow; }
#endif
	void setCapture(Widget* inter) noexcept;
	void resetLayouts();
	void clearLayouts() noexcept;
	void setLayouts();
	RootLayout* getLayout() noexcept { return layout; }
	Overlay* getOverlay() noexcept { return overlay; }
	Popup* getPopup() noexcept { return popup; }
	Popup* releasePopup() noexcept;	// can only be used for popups without capture or contexts
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	Context* getContext() noexcept { return context; }
	void setContext(Context* newContext);

	void updateSelect() noexcept;
	void updateSelect(Widget* sel) noexcept;
	void deselect() noexcept;
	void selectFirst() noexcept;
	bool cursorInClickRange(ivec2 mPos, uint8 mBut) noexcept;

	ScrollArea* getSelectedScrollArea() const noexcept;
private:
	Widget* getSelected(ivec2 mPos) noexcept;
	bool overlayFocused(ivec2 mPos) const noexcept;
	void finishNewPopup(Popup* lay);
};

inline bool Scene::cursorInClickRange(ivec2 mPos, uint8 mBut) noexcept {
	return glm::length(vec2(mPos - stamps[mBut - 1].mPos)) <= clickMoveThreshold;
}
