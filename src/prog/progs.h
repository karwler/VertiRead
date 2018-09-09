#pragma once

#include "utils/layouts.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	void eventEnter();
	virtual void eventEscape() {}
	virtual void eventUp();
	virtual void eventDown();
	virtual void eventLeft();
	virtual void eventRight();
	virtual void eventScrollUp(float amt) {}
	virtual void eventScrollDown(float amt) {}
	virtual void eventScrollLeft(float amt) {}
	virtual void eventScrollRight(float amt) {}
	void eventCursorUp(float amt);
	void eventCursorDown(float amt);
	void eventCursorLeft(float amt);
	void eventCursorRight(float amt);
	virtual void eventCenterView() {}
	virtual void eventNextPage() {}
	virtual void eventPrevPage() {}
	virtual void eventZoomIn() {}
	virtual void eventZoomOut() {}
	virtual void eventZoomReset() {}
	virtual void eventToStart() {}
	virtual void eventToEnd() {}
	virtual void eventNextDir() {}
	virtual void eventPrevDir() {}
	virtual void eventFullscreen();
	void eventRefresh();

	virtual void eventFileDrop(const string& file) {}
	virtual void eventClosing() {}
	
	virtual Layout* createLayout() { return nullptr; }
	virtual Overlay* createOverlay() { return nullptr; }
	static Popup* createPopupMessage(const string& msg, const vec2<Size>& size=messageSize);
	
protected:
	struct Text {
		Text(const string& str, int height, int margin=Default::textMargin);

		string text;
		int length;
	};
	static int findMaxLength(const vector<string>& strs, int height, int margin=Default::textMargin);

	static const int lineHeight;
	static const int topHeight;
	static const int topSpacing;
	static const int picSize;
	static const int picMargin;
	static const vec2s messageSize;

	bool tryClosePopup();
private:
	void eventSelect(const Direction& dir);
};

class ProgBooks : public ProgState {
public:
	virtual void eventEscape() override;
	
	virtual void eventFileDrop(const string& file) override;

	virtual Layout* createLayout() override;
};

class ProgPageBrowser : public ProgState {
public:
	virtual void eventEscape() override;

	virtual void eventFileDrop(const string& file) override;

	virtual Layout* createLayout() override;
};

class ProgReader : public ProgState {
public:
	virtual void eventEscape() override;
	virtual void eventUp() override;
	virtual void eventDown() override;
	virtual void eventLeft() override;
	virtual void eventRight() override;
	virtual void eventScrollUp(float amt) override;
	virtual void eventScrollDown(float amt) override;
	virtual void eventScrollLeft(float amt) override;
	virtual void eventScrollRight(float amt) override;
	virtual void eventCenterView() override;
	virtual void eventNextPage() override;
	virtual void eventPrevPage() override;
	virtual void eventZoomIn() override;
	virtual void eventZoomOut() override;
	virtual void eventZoomReset() override;
	virtual void eventToStart() override;
	virtual void eventToEnd() override;
	virtual void eventNextDir() override;
	virtual void eventPrevDir() override;

	virtual void eventClosing() override;

	virtual Layout* createLayout() override;
	virtual Overlay* createOverlay() override;

private:
	float modifySpeed(float value);	// change scroll speed depending on pressed bindings
};

class ProgSettings : public ProgState {
public:
	virtual void eventEscape() override;
	virtual void eventFullscreen() override;

	virtual void eventFileDrop(const string& file) override;
	
	virtual Layout* createLayout() override;
};

class ProgSearchDir : public ProgState {
public:
	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
};
