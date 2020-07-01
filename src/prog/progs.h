#pragma once

#include "utils/layouts.h"
#include "downloader.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	static constexpr char dotStr[] = ".";

protected:
	struct Text {
		string text;
		int length;

		Text(string str, int height, int margin = Label::defaultTextMargin);
	};

	static constexpr int popupLineHeight = 40;
	static constexpr int tooltipHeight = 16;
	static constexpr int lineHeight = 30;
	static constexpr int topHeight = 40;
	static constexpr int topSpacing = 10;
	static constexpr int picSize = 40;
	static constexpr int picMargin = 4;
private:
	static constexpr float cursorMoveFactor = 10.f;

	uint maxTooltipLength;

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
	virtual void eventHide();
	void eventBoss();
	void eventRefresh();
	virtual void eventFileDrop(const fs::path&) {}
	virtual void eventClosing() {}
	void onResize();

	virtual RootLayout* createLayout();
	virtual Overlay* createOverlay();
	static Popup* createPopupMessage(string msg, PCall ccal, string ctxt = "Okay", Label::Alignment malign = Label::Alignment::left);
	static Popup* createPopupChoice(string msg, PCall kcal, PCall ccal, Label::Alignment malign = Label::Alignment::left);

protected:
	template <class T> static int findMaxLength(T pos, T end, int height, int margin = Label::defaultTextMargin);	// TODO: no margin parameter
	SDL_Texture* makeTooltip(const char* str);
	SDL_Texture* makeTooltipL(const char* str);

private:
	void eventSelect(Direction dir);
};

inline ProgState::ProgState() {
	onResize();
}

class ProgBooks : public ProgState {
public:
	virtual ~ProgBooks() override = default;

	virtual void eventEscape() override;
	virtual void eventHide() override;
	virtual void eventFileDrop(const fs::path& file) override;

	virtual RootLayout* createLayout() override;
};

class ProgPageBrowser : public ProgState {
public:
	virtual ~ProgPageBrowser() override = default;

	virtual void eventEscape() override;
	virtual void eventHide() override;
	virtual void eventFileDrop(const fs::path& file) override;

	virtual RootLayout* createLayout() override;
};

class ProgReader : public ProgState {
public:
	ReaderBox* reader;
private:
	static constexpr float scrollFactor = 2.f;

public:
	virtual ~ProgReader() override = default;

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
	virtual void eventHide() override;
	virtual void eventClosing() override;

	virtual RootLayout* createLayout() override;
	virtual Overlay* createOverlay() override;

private:
	int modifySpeed(float value);	// change scroll speed depending on pressed bindings
};
#ifdef BUILD_DOWNLOADER
class ProgDownloader : public ProgState {
public:
	LabelEdit* query;
	ScrollArea* results;
	ScrollArea* chapters;
	CheckBox* chaptersTick;

	vector<string> resultUrls, chapterUrls;

public:
	virtual ~ProgDownloader() override = default;

	virtual void eventEscape() override;

	virtual RootLayout* createLayout() override;
	Comic curInfo() const;
	void printResults(vector<pairStr>&& comics);
	void printInfo(vector<pairStr>&& chaps);
};

class ProgDownloads : public ProgState {
public:
	ScrollArea* list;

public:
	virtual ~ProgDownloads() override = default;

	virtual void eventEscape() override;

	virtual RootLayout* createLayout() override;
};
#endif
class ProgSettings : public ProgState {
public:
	fs::path oldPathBuffer;	// for keeping old library path between decisions
	Layout* limitLine;
	Slider* deadzoneSL;
	LabelEdit* deadzoneLE;
private:
	CheckBox* fullscreen;
	CheckBox* showHidden;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	virtual void eventFullscreen() override;
	virtual void eventHide() override;
	virtual void eventFileDrop(const fs::path& file) override;

	virtual RootLayout* createLayout() override;

	Widget* createLimitEdit();
};

class ProgSearchDir : public ProgState {
public:
	ScrollArea* list;

public:
	virtual ~ProgSearchDir() override = default;

	virtual void eventEscape() override;
	virtual void eventHide() override;

	virtual RootLayout* createLayout() override;
};
