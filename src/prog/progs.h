#pragma once

#include "utils/layouts.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	virtual void eventBack() {}
	virtual void eventScreenMode();
	virtual void eventUp(float amt) {}
	virtual void eventDown(float amt) {}
	virtual void eventRight(float amt) {}
	virtual void eventLeft(float amt) {}
	virtual void eventPageUp() {}
	virtual void eventPageDown() {}
	virtual void eventZoomIn() {}
	virtual void eventZoomOut() {}
	virtual void eventZoomReset() {}
	virtual void eventCenterView() {}
	virtual void eventNextDir() {}
	virtual void eventPrevDir() {}
	virtual void eventPlayPause() {}
	virtual void eventNextSong() {}
	virtual void eventPrevSong() {}
	virtual void eventVolumeUp() {}
	virtual void eventVolumeDown() {}
	virtual void eventMute() {}

	virtual void eventFileDrop(char* file) {}
	virtual void eventClosing() {}
	
	virtual Layout* createLayout() { return nullptr; }
	virtual vector<Overlay*> createOverlays() { return {}; }
	static Popup* createPopupMessage(const string& msg, const vec2<Size>& size=messageSize);
	static Popup* createPopupChoice(const string& msg, void (Program::*call)(Button*), const vec2<Size>& size=messageSize);
	static pair<Popup*, LineEdit*> createPopupTextInput(const string& msg, const string& txt, void (Program::*call)(Button*), LineEdit::TextType type=LineEdit::TextType::text, const vec2<Size>& size=inputSize);

protected:
	static const int topHeight;
	static const int topSpacing;
	static const vec2i optSize;
	static const int itemHeight;
	static const int sideWidth;
	static const int picSize;
	static const int picSpaer;
	static const int setsDescLength;
	static const vec2s messageSize;
	static const vec2s inputSize;
};

class ProgBooks : public ProgState {
public:
	virtual void eventBack();
	
	virtual Layout* createLayout();
};

class ProgBrowser : public ProgState {
public:
	virtual void eventBack();

	virtual Layout* createLayout();
};

class ProgReader : public ProgState {
public:
	virtual void eventBack();
	virtual void eventUp(float amt);
	virtual void eventDown(float amt);
	virtual void eventRight(float amt);
	virtual void eventLeft(float amt);
	virtual void eventPageUp();
	virtual void eventPageDown();
	virtual void eventZoomIn();
	virtual void eventZoomOut();
	virtual void eventZoomReset();
	virtual void eventCenterView();
	virtual void eventNextDir();
	virtual void eventPrevDir();
	virtual void eventPlayPause();
	virtual void eventNextSong();
	virtual void eventPrevSong();
	virtual void eventVolumeUp();
	virtual void eventVolumeDown();
	virtual void eventMute();

	virtual void eventClosing();

	virtual Layout* createLayout();
	virtual vector<Overlay*> createOverlays();

private:
	float modifySpeed(float value);	// change scroll speed depending on pressed bindings
};

class ProgPlaylists : public ProgState {
public:
	virtual void eventBack();

	virtual Layout* createLayout();
};

class ProgEditor : public ProgState {
public:
	virtual void eventBack();
	virtual void eventFileDrop(char* file);

	virtual Layout* createLayout();
};

class ProgSearchSongs : public ProgState {
public:
	virtual void eventBack();

	virtual Layout* createLayout();
};

class ProgSearchBooks : public ProgState {
public:
	virtual void eventBack();

	virtual Layout* createLayout();
};

class ProgSettings : public ProgState {
public:
	virtual void eventBack();
	
	virtual Layout* createLayout();
};
