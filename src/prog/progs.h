#pragma once

#include "downloader.h"
#include "utils/settings.h"
#include <atomic>
#include <thread>

// for handling program state specific things that occur in all states
class ProgState {
public:
	static constexpr char dotStr[] = ".";

protected:
	struct Text {
		string text;
		uint length;

		Text(string&& str, uint height);

		static uint measure(string_view str, uint height);
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
	virtual ~ProgState() = default;

	void eventEnter();
	void eventEscape();
	virtual void eventSpecEscape() {}
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
	virtual void eventRefresh();
	virtual void eventFileDrop(const fs::path&) {}
	virtual void eventClosing() {}
	void onResize();

	virtual RootLayout* createLayout() = 0;
	virtual Overlay* createOverlay();
	Popup* createPopupMessage(string msg, PCall ccal, string ctxt = "Okay", Alignment malign = Alignment::left);
	Popup* createPopupMultiline(string msg, PCall ccal, string ctxt = "Okay", Alignment malign = Alignment::left);
	Popup* createPopupChoice(string msg, PCall kcal, PCall ccal, Alignment malign = Alignment::left);
	void updatePopupMessage(string msg);
	Context* createContext(vector<pair<string, PCall>>&& items, Widget* parent);
	Context* createComboContext(ComboBox* parent, PCall kcal);

	int getLineHeight() const;
	static Recti calcTextContextRect(const vector<Widget*>& items, ivec2 pos, ivec2 size, int margin);
protected:
	template <class T> static int findMaxLength(T pos, T end, int height);
	Texture* makeTooltip(const char* str);
	Texture* makeTooltipL(const char* str);

private:
	void eventSelect(Direction dir);
	static void calcContextPos(int& pos, int& siz, int limit);
};

inline ProgState::ProgState() {
	onResize();
}

inline int ProgState::getLineHeight() const {
	return lineHeight;
}

class ProgFileExplorer : public ProgState {
public:
	LabelEdit* locationBar = nullptr;
	ScrollArea* fileList;
	size_t dirEnd, fileEnd;

	~ProgFileExplorer() override = default;

	void eventHide() final;
	void eventRefresh() final;
	void processFileChanges(const Browser* browser, vector<pair<bool, string>>& files, bool gone);

protected:
	virtual Label* makeDirectoryEntry(const Size& size, string&& name) = 0;
	virtual Label* makeFileEntry(const Size& size, string&& name);
	virtual Size fileEntrySize(string_view name);
};

class ProgBooks : public ProgFileExplorer {
public:
	~ProgBooks() final = default;

	void eventSpecEscape() final;
	void eventFileDrop(const fs::path& file) final;

	RootLayout* createLayout() final;
protected:
	Label* makeDirectoryEntry(const Size& size, string&& name) final;
	Size fileEntrySize(string_view name) final;
};

class ProgPageBrowser : public ProgFileExplorer {
public:
	~ProgPageBrowser() final = default;

	void eventSpecEscape() final;
	void eventFileDrop(const fs::path& file) final;
	void resetFileIcons();

	RootLayout* createLayout() final;
protected:
	Label* makeDirectoryEntry(const Size& size, string&& name) final;
	Label* makeFileEntry(const Size& size, string&& name) final;
};

class ProgReader : public ProgState {
public:
	ReaderBox* reader;
private:
	static constexpr float scrollFactor = 2.f;

public:
	~ProgReader() final = default;

	void eventSpecEscape() final;
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
	void eventRefresh() final;
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
	void printResults(vector<pair<string, string>>&& comics);
	void printInfo(vector<pair<string, string>>&& chaps);
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
	std::thread thread;
	std::atomic<ThreadType> threadType;

	Layout* limitLine;
private:
	ComboBox* screen;
	CheckBox* showHidden;
	vector<pair<u32vec2, string>> devices;

public:
	~ProgSettings() final;

	void eventSpecEscape() final;
	void eventFullscreen() final;
	void eventMultiFullscreen() final;
	void eventHide() final;
	void eventRefresh() final;
	void eventFileDrop(const fs::path& file) final;

	RootLayout* createLayout() final;
	Widget* createLimitEdit();
	u32vec2 getDvice(size_t id) const;

	void startMove();
	void stopMove();
	static void logMoveErrors(const string* errors);
};

inline u32vec2 ProgSettings::getDvice(size_t id) const {
	return devices[id].first;
}

class ProgSearchDir : public ProgFileExplorer {
public:
	~ProgSearchDir() final = default;

	void eventSpecEscape() final;

	RootLayout* createLayout() final;
protected:
	Label* makeDirectoryEntry(const Size& size, string&& name) final;
};
