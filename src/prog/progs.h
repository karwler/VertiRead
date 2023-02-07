#pragma once

#include "downloader.h"
#include "utils/settings.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	static constexpr char dotStr[] = ".";

protected:
	struct Text {
		string text;
		int length;

		Text(string str, int height);
	};

	int popupLineHeight;
	int tooltipHeight;
	int lineHeight;
	int topHeight;
	int topSpacing;
	int picSize;
	int picMargin;
private:
	int contextMargin;
	uint maxTooltipLength;
	float cursorMoveFactor;

public:
	ProgState();
	virtual ~ProgState() = default;	// to keep the compiler happy

	void eventEnter();
	virtual void eventEscape() {}
	virtual void eventUp();
	virtual void eventDown();
	virtual void eventLeft();
	virtual void eventRight();
	virtual void eventScrollUp(float) {}
	virtual void eventScrollDown(float) {}
	virtual void eventScrollLeft(float) {}
	virtual void eventScrollRight(float) {}
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
	virtual void eventMultiFullscreen();
	virtual void eventHide();
	void eventBoss();
	void eventRefresh();
	virtual void eventFileDrop(const fs::path&) {}
	virtual void eventClosing() {}
	void onResize();

	virtual RootLayout* createLayout() = 0;
	virtual Overlay* createOverlay();
	Popup* createPopupMessage(string msg, PCall ccal, string ctxt = "Okay", Alignment malign = Alignment::left);
	Popup* createPopupChoice(string msg, PCall kcal, PCall ccal, Alignment malign = Alignment::left);
	Context* createContext(vector<pair<string, PCall>>&& items, Widget* parent);
	Context* createComboContext(ComboBox* parent, PCall kcal);

	static Recti calcTextContextRect(const vector<Widget*>& items, ivec2 pos, ivec2 size, int margin);
protected:
	template <class T> static int findMaxLength(T pos, T end, int height);
	Texture* makeTooltip(const char* str);
	Texture* makeTooltipL(const char* str);

	bool eventCommonEscape();	// returns true if something happened
private:
	void eventSelect(Direction dir);
	static void calcContextPos(int& pos, int& siz, int limit);
};

inline ProgState::ProgState() {
	onResize();
}

class ProgBooks : public ProgState {
public:
	~ProgBooks() final = default;

	void eventEscape() final;
	void eventHide() final;
	void eventFileDrop(const fs::path& file) final;

	RootLayout* createLayout() final;
};

class ProgPageBrowser : public ProgState {
public:
	ScrollArea* fileList;

	~ProgPageBrowser() final = default;

	void eventEscape() final;
	void eventHide() final;
	void eventFileDrop(const fs::path& file) final;

	RootLayout* createLayout() final;
};

class ProgReader : public ProgState {
public:
	ReaderBox* reader;
private:
	static constexpr float scrollFactor = 2.f;

public:
	~ProgReader() final = default;

	void eventEscape() final;
	void eventUp() final;
	void eventDown() final;
	void eventLeft() final;
	void eventRight() final;
	void eventScrollUp(float amt) final;
	void eventScrollDown(float amt) final;
	void eventScrollLeft(float amt) final;
	void eventScrollRight(float amt) final;
	void eventCenterView() final;
	void eventNextPage() final;
	void eventPrevPage() final;
	void eventZoomIn() final;
	void eventZoomOut() final;
	void eventZoomReset() final;
	void eventToStart() final;
	void eventToEnd() final;
	void eventNextDir() final;
	void eventPrevDir() final;
	void eventHide() final;
	void eventClosing() final;

	RootLayout* createLayout() final;
	Overlay* createOverlay() final;

private:
	static int modifySpeed(float value);	// change scroll speed depending on pressed bindings
};

#ifdef DOWNLOADER
class ProgDownloader : public ProgState {
public:
	LabelEdit* query;
	ScrollArea* results;
	ScrollArea* chapters;
	CheckBox* chaptersTick;

	vector<string> resultUrls, chapterUrls;

public:
	~ProgDownloader() final = default;

	void eventEscape() final;

	RootLayout* createLayout() final;
	Comic curInfo() const;
	void printResults(vector<pairStr>&& comics);
	void printInfo(vector<pairStr>&& chaps);
};

class ProgDownloads : public ProgState {
public:
	ScrollArea* list;

public:
	~ProgDownloads() final = default;

	void eventEscape() final;

	RootLayout* createLayout() final;
};
#endif

class ProgSettings : public ProgState {
public:
	fs::path oldPathBuffer;	// for keeping old library path between decisions
	Layout* limitLine;
	Slider* deadzoneSL;
	LabelEdit* deadzoneLE;
private:
	ComboBox* screen;
	CheckBox* showHidden;
	vector<pair<u32vec2, string>> devices;

public:
	~ProgSettings() final = default;

	void eventEscape() final;
	void eventFullscreen() final;
	void eventMultiFullscreen() final;
	void eventHide() final;
	void eventFileDrop(const fs::path& file) final;

	RootLayout* createLayout() final;

	Widget* createLimitEdit();
	u32vec2 getDvice(sizet id) const;
};

inline u32vec2 ProgSettings::getDvice(sizet id) const {
	return devices[id].first;
}

class ProgSearchDir : public ProgState {
public:
	ScrollArea* list;

	~ProgSearchDir() final = default;

	void eventEscape() final;
	void eventHide() final;

	RootLayout* createLayout() final;
};
