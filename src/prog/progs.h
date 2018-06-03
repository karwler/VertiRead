#pragma once

#include "utils/layouts.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	virtual void eventEnter();
	virtual void eventEscape() {}
	virtual void eventUp();
	virtual void eventDown();
	virtual void eventLeft();
	virtual void eventRight();
	virtual void eventScrollUp(float amt) {}
	virtual void eventScrollDown(float amt) {}
	virtual void eventScrollLeft(float amt) {}
	virtual void eventScrollRight(float amt) {}
	virtual void eventCursorUp(float amt);
	virtual void eventCursorDown(float amt);
	virtual void eventCursorLeft(float amt);
	virtual void eventCursorRight(float amt);
	virtual void eventCenterView() {}
	virtual void eventPageUp() {}
	virtual void eventPageDown() {}
	virtual void eventZoomIn() {}
	virtual void eventZoomOut() {}
	virtual void eventZoomReset() {}
	virtual void eventNextDir() {}
	virtual void eventPrevDir() {}
	virtual void eventFullscreen();

	virtual void eventFileDrop(char* file) {}
	virtual void eventClosing() {}
	
	virtual Layout* createLayout() { return nullptr; }
	virtual Overlay* createOverlay() { return nullptr; }
	static Popup* createPopupMessage(const string& msg, const vec2<Size>& size=messageSize);
	
protected:
	struct Text {
		Text() {}
		Text(const string& TXT, int H);

		string text;
		int length;
	};
	static int findMaxLength(const vector<string>& strs, int height);

	static const int lineHeight;
	static const int topHeight;
	static const int topSpacing;
	static const int picSize;
	static const int picMargin;
	static const int iconMargin;
	static const vec2s messageSize;
};

class ProgBooks : public ProgState {
public:
	virtual void eventEscape();
	
	virtual Layout* createLayout();
};

class ProgPageBrowser : public ProgState {
public:
	virtual void eventEscape();

	virtual Layout* createLayout();
};

class ProgReader : public ProgState {
public:
	virtual void eventEscape();
	virtual void eventUp();
	virtual void eventDown();
	virtual void eventLeft();
	virtual void eventRight();
	virtual void eventScrollUp(float amt);
	virtual void eventScrollDown(float amt);
	virtual void eventScrollLeft(float amt);
	virtual void eventScrollRight(float amt);
	virtual void eventCenterView();
	virtual void eventPageUp();
	virtual void eventPageDown();
	virtual void eventZoomIn();
	virtual void eventZoomOut();
	virtual void eventZoomReset();
	virtual void eventNextDir();
	virtual void eventPrevDir();

	virtual void eventClosing();

	virtual Layout* createLayout();
	virtual Overlay* createOverlay();

private:
	float modifySpeed(float value);	// change scroll speed depending on pressed bindings
};

class ProgSettings : public ProgState {
public:
	virtual void eventEscape();
	virtual void eventFullscreen();
	
	virtual Layout* createLayout();
};

class ProgSearchDir : public ProgState {
public:
	virtual void eventEscape();

	virtual Layout* createLayout();
};
