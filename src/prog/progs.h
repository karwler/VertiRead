#pragma once

#include "types.h"
#include "utils/settings.h"
#include <span>
#include <thread>

// for handling program state specific things that occur in all states
class ProgState {
public:
	static constexpr char dotStr[] = ".";

protected:
	int popupLineHeight;
	int tooltipHeight;
	int lineHeight;
	int topHeight;
	int topSpacing;
	int picSize;
private:
	int contextMargin;
	uint maxTooltipLength;
	float cursorMoveFactor;

public:
	ProgState() { onResize(); }
	virtual ~ProgState() = default;

	template <MemberFunction F, class... A> void exec(F func, A&&... args);
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
	virtual void eventZoomFit() {}
	virtual void eventToStart() {}
	virtual void eventToEnd() {}
	virtual void eventNextDir() {}
	virtual void eventPrevDir() {}
	virtual void eventFullscreen();
	virtual void eventMultiFullscreen();
	virtual void eventHide();
	void eventBoss();
	virtual void eventRefresh();
	virtual void eventFileDrop(const char*) {}
	virtual void eventClosing() {}
	void onResize();

	virtual RootLayout* createLayout() = 0;
	virtual Overlay* createOverlay();
	Popup* createPopupMessage(Cstring&& msg, EventId ccal = GeneralEvent::closePopup, Cstring&& ctxt = "Okay", Alignment malign = Alignment::left);
	void updatePopupMessage(Cstring&& msg);
	Popup* createPopupMultiline(Cstring&& msg, EventId ccal = GeneralEvent::closePopup, Cstring&& ctxt = "Okay");
	Popup* createPopupChoice(Cstring&& msg, EventId kcal, EventId ccal = GeneralEvent::closePopup, Alignment malign = Alignment::left);
	Popup* createPopupInput(Cstring&& msg, string&& text, EventId kcal, EventId ccal = GeneralEvent::closePopup, Cstring&& ktxt = "Okay", Alignment malign = Alignment::left);
	static const string& inputFromPopup();
	Popup* createPopupRemoteLogin(RemoteLocation&& rl, EventId kcall, EventId ccal = GeneralEvent::closePopup);
	static pair<RemoteLocation, bool> remoteLocationFromPopup();
	Context* createContext(vector<pair<Cstring, EventId> >&& items, Widget* parent);
	Context* createComboContext(ComboBox* parent, EventId kcal);

	int getLineHeight() const { return lineHeight; }
	pair<int, uint> getTooltipParams() const { return pair(tooltipHeight, maxTooltipLength); }
	static Recti calcTextContextRect(const Children& items, ivec2 pos, ivec2 size, int margin);
protected:
	static uint measureText(string_view str, uint height);
	template <Iterator T> static uint findMaxLength(T pos, T end, uint height);

private:
	void eventSelect(Direction dir);
	static void calcContextPos(int& pos, int& siz, int limit);
};

template <MemberFunction F, class... A>
void ProgState::exec(F func, A&&... args) {
	if (func)
		(this->*func)(std::forward<A>(args)...);
}

class ProgFileExplorer : public ProgState {
private:
	vector<FileChange> fileChanges;
public:
	LabelEdit* locationBar = nullptr;
	ScrollArea* fileList;
	size_t dirEnd, fileEnd;

	~ProgFileExplorer() override = default;

	void eventHide() override;
	void eventRefresh() override;
	void processFileChanges(Browser* browser);

protected:
	virtual PushButton* makeDirectoryEntry(const Size& size, Cstring&& name) = 0;
	virtual PushButton* makeFileEntry(const Size& size, Cstring&& name);
	virtual Size fileEntrySize(string_view name);
};

class ProgBooks final : public ProgFileExplorer {
public:
	PushButton* contextBook;	// for keeping track of right clicked button

	~ProgBooks() override = default;

	void eventSpecEscape() override;
	void eventFileDrop(const char* file) override;

	RootLayout* createLayout() override;
	PushButton* makeBookTile(Cstring&& name);
protected:
	PushButton* makeDirectoryEntry(const Size& size, Cstring&& name) override;
	Size fileEntrySize(string_view name) override;
};

class ProgPageBrowser final : public ProgFileExplorer {
public:
	~ProgPageBrowser() override;

	void eventSpecEscape() override;
	void eventFileDrop(const char* file) override;
	void resetFileIcons();

	RootLayout* createLayout() override;
protected:
	PushButton* makeDirectoryEntry(const Size& size, Cstring&& name) override;
	PushButton* makeFileEntry(const Size& size, Cstring&& name) override;
};

class ProgReader final : public ProgState {
public:
	ReaderBox* reader;
private:
	static constexpr float scrollFactor = 2.f;

public:
	~ProgReader() override = default;

	void eventSpecEscape() override;
	void eventUp() override;
	void eventDown() override;
	void eventLeft() override;
	void eventRight() override;
	void eventScrollUp(float amt) override;
	void eventScrollDown(float amt) override;
	void eventScrollLeft(float amt) override;
	void eventScrollRight(float amt) override;
	void eventCenterView() override;
	void eventNextPage() override;
	void eventPrevPage() override;
	void eventZoomIn() override;
	void eventZoomOut() override;
	void eventZoomReset() override;
	void eventZoomFit() override;
	void eventToStart() override;
	void eventToEnd() override;
	void eventNextDir() override;
	void eventPrevDir() override;
	void eventHide() override;
	void eventRefresh() override;
	void eventClosing() override;

	RootLayout* createLayout() override;
	Overlay* createOverlay() override;

private:
	Cstring makeTooltipWithKey(const char* text, Binding::Type type);
	static int modifySpeed(float value);	// change scroll speed depending on pressed bindings
};

class ProgSettings final : public ProgState {
public:
	string oldPathBuffer;	// for keeping old library path between decisions
	LabelEdit* libraryDir;
	Layout* limitLine;
private:
	ComboBox* screen;
	CheckBox* showHidden;
	ComboBox* fontList;
	vector<pair<u32vec2, string>> devices;
	std::jthread moveThread, fontThread;

public:
	~ProgSettings() override;

	void eventSpecEscape() override;
	void eventFullscreen() override;
	void eventMultiFullscreen() override;
	void eventHide() override;
	void eventRefresh() override;
	void eventFileDrop(const char* file) override;

	RootLayout* createLayout() override;
	Widget* createLimitEdit();
	u32vec2 getDevice(size_t id) const { return devices[id].first; }
	static Cstring makeZoomText();

	void stopFonts();
	void setFontField(vector<Cstring>&& families, uptr<Cstring[]>&& files, size_t select);
	void startMove();
	void stopMove();
	static void logMoveErrors(const string* errors);
private:
	void startFonts();
};

class ProgSearchDir final : public ProgFileExplorer {
public:
	PushButton* selected = nullptr;

	~ProgSearchDir() override = default;

	void eventSpecEscape() override;

	RootLayout* createLayout() override;
protected:
	PushButton* makeDirectoryEntry(const Size& size, Cstring&& name) override;
};
