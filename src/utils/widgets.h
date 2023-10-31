#pragma once

#include "settings.h"

// scroll information
class Scrollable {
public:
	static constexpr int barSizeVal = 10;

	ivec2 listPos = ivec2(0);
	vec2 motion = vec2(0.f);	// how much the list scrolls over time
private:
	ivec2 listSize;
	ivec2 listMax;
	int sliderSize;
	int sliderMax;
	int diffSliderMouse = 0;	// space between slider and mouse position
	bool draggingSlider = false;

	static constexpr float throttle = 10.f;

public:
	bool tick(float dSec);	// returns whether the list has moved
	bool hold(ivec2 mPos, uint8 mBut, Widget* wgt, ivec2 pos, ivec2 size, bool vert);	// returns whether the list has moved
	void drag(ivec2 mPos, ivec2 mMov, ivec2 pos, bool vert);
	void undrag(ivec2 mPos, uint8 mBut, bool vert);
	void scroll(ivec2 wMov, bool vert);

	bool getDraggingSlider() const;
	void setLimits(ivec2 lsize, ivec2 wsize, bool vert);
	ivec2 getListSize() const;
	ivec2 getListMax() const;
	void setListPos(ivec2 pos);
	void moveListPos(ivec2 mov);
	int barSize(ivec2 wsize, bool vert) const;	// returns 0 if slider isn't needed
	Recti barRect(ivec2 pos, ivec2 size, bool vert) const;
	Recti sliderRect(ivec2 pos, ivec2 size, bool vert) const;
private:
	void setSlider(int spos, ivec2 pos, bool vert);
	int sliderPos(ivec2 pos, ivec2 wsize, bool vert) const;
	static void throttleMotion(float& mov, float dSec);
};

inline bool Scrollable::getDraggingSlider() const {
	return draggingSlider;
}

inline ivec2 Scrollable::getListSize() const {
	return listSize;
}

inline ivec2 Scrollable::getListMax() const {
	return listMax;
}

inline void Scrollable::setListPos(ivec2 pos) {
	listPos = glm::clamp(pos, ivec2(0), listMax);
}

inline void Scrollable::moveListPos(ivec2 mov) {
	setListPos(listPos + mov);
}

inline int Scrollable::barSize(ivec2 wsize, bool vert) const {
	return listSize[vert] > wsize[vert] ? barSizeVal : 0;
}

inline int Scrollable::sliderPos(ivec2 pos, ivec2 wsize, bool vert) const {
	return listSize[vert] > wsize[vert] ? pos[vert] + listPos[vert] * sliderMax / listMax[vert] : 0;
}

// can be used as spacer
class Widget {
protected:
	Layout* parent = nullptr;	// every widget that isn't a Layout should have a parent
	Size relSize;				// size relative to parent's parameters

public:
	Widget(const Size& size = Size());
	virtual ~Widget() = default;

	virtual void drawSelf(const Recti&) {}	// calls appropriate drawing function(s) in DrawSys
	virtual void drawTop(const Recti&) const {}
	virtual void drawAddr(const Recti&) {}
	virtual void onResize() {}	// for updating values when window size changed
	virtual void tick(float) {}
	virtual void postInit() {}	// gets called after parent is set and all set up
	virtual void onClick(ivec2, uint8) {}
	virtual void onDoubleClick(ivec2, uint8) {}
	virtual void onMouseMove(ivec2, ivec2) {}
	virtual void onHold(ivec2, uint8) {}
	virtual void onDrag(ivec2, ivec2) {}	// mouse move while left button down
	virtual void onUndrag(ivec2, uint8) {}	// gets called on mouse button up if instance is Scene's capture
	virtual void onScroll(ivec2) {}	// on mouse wheel y movement
	virtual void onKeypress(const SDL_Keysym&) {}
	virtual void onJButton(uint8) {}
	virtual void onJHat(uint8, uint8) {}
	virtual void onJAxis(uint8, bool) {}
	virtual void onGButton(SDL_GameControllerButton) {}
	virtual void onGAxis(SDL_GameControllerAxis, bool) {}
	virtual void onCompose(string_view, uint) {}
	virtual void onText(string_view, uint) {}
	virtual void onDisplayChange() {}
	virtual void onNavSelect(Direction dir);
	virtual bool navSelectable() const;
	virtual bool hasDoubleclick() const;

	Layout* getParent() const;
	void setParent(Layout* pnt, uint id);
	uint getIndex() const;
	const Size& getRelSize() const;
	virtual ivec2 position() const;
	virtual ivec2 size() const;
	ivec2 center() const;
	Recti rect() const;			// the rectangle that is the widget
	virtual Recti frame() const;	// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)
	virtual void setSize(const Size& size);
	int sizeToPixAbs(const Size& siz, int res) const;
};

inline Widget::Widget(const Size& size) :
	relSize(size)
{}

inline Layout* Widget::getParent() const {
	return parent;
}

inline uint Widget::getIndex() const {
	return relSize.id;
}

inline const Size& Widget::getRelSize() const {
	return relSize;
}

inline ivec2 Widget::center() const {
	return position() + size() / 2;
}

inline Recti Widget::rect() const {
	return Recti(position(), size());
}

// visible widget with texture and background color
class Picture : public Widget {
public:
	static constexpr int defaultIconMargin = 2;
	static constexpr pair<Texture*, bool> nullTex = pair(nullptr, false);

private:
	Texture* tex;
	bool freeTex = false;
public:
	bool showBG;
protected:
	int texMargin;

public:
	Picture(const Size& size = Size(), bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~Picture() override;

	void drawSelf(const Recti& view) override;
	void drawAddr(const Recti& view) override;

	virtual Color color() const;
	virtual Recti texRect() const;
	Texture* getTex() const;
	void setTex(Texture* texture, bool exclusive);
};

inline Texture* Picture::getTex() const {
	return tex;
}

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Picture {
public:
	static constexpr ivec2 tooltipMargin = ivec2(4, 1);

protected:
	PCall lcall, rcall, dcall;
	Texture* tooltip;

public:
	Button(const Size& size = Size(), PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~Button() override;

	void onClick(ivec2 mPos, uint8 mBut) override;
	void onDoubleClick(ivec2 mPos, uint8 mBut) override;
	bool navSelectable() const override;
	bool hasDoubleclick() const override;

	Color color() const override;
	virtual const Texture* getTooltip();
	Recti tooltipRect() const;
	void setCalls(optional<PCall> lc, optional<PCall> rc, optional<PCall> dc);
};

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox final : public Button {
public:
	bool on;

	CheckBox(const Size& size = Size(), bool checked = false, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~CheckBox() final = default;

	void drawSelf(const Recti& view) final;
	void onClick(ivec2 mPos, uint8 mBut) final;

	Recti boxRect() const;
	Color boxColor() const;
	bool toggle();
};

inline Color CheckBox::boxColor() const {
	return on ? Color::light : Color::dark;
}

inline bool CheckBox::toggle() {
	return on = !on;
}

// horizontal slider (maybe one day it'll be able to be vertical)
class Slider final : public Button {
private:
	int val, vmin, vmax;
	int diffSliderMouse = 0;

public:
	Slider(const Size& size = Size(), int value = 0, int minimum = 0, int maximum = 255, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~Slider() final = default;

	void drawSelf(const Recti& view) final;
	void onClick(ivec2 mPos, uint8 mBut) final;
	void onHold(ivec2 mPos, uint8 mBut) final;
	void onDrag(ivec2 mPos, ivec2 mMov) final;
	void onUndrag(ivec2 mPos, uint8 mBut) final;

	int getVal() const;
	void setVal(int value);

	Recti barRect() const;
	Recti sliderRect() const;

private:
	void setSlider(int xpos);
	int sliderPos() const;
	int sliderLim() const;
};

inline int Slider::getVal() const {
	return val;
}

inline void Slider::setVal(int value) {
	val = std::clamp(value, vmin, vmax);
}

inline int Slider::sliderPos() const {
	return position().x + size().y/4 + val * sliderLim() / vmax;
}

// horizontal progress bar
class ProgressBar final : public Picture {
private:
	static constexpr int barMarginFactor = 8;

	int val, vmin, vmax;

public:
	ProgressBar(const Size& size = Size(), int value = 0, int minimum = 0, int maximum = 255, bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~ProgressBar() final = default;

	void drawSelf(const Recti& view) final;

	int getVal() const;
	void setVal(int value);

	Recti barRect() const;
};

inline int ProgressBar::getVal() const {
	return val;
}

inline void ProgressBar::setVal(int value) {
	val = std::clamp(value, vmin, vmax);
}

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	static constexpr int defaultTextMargin = 5;

protected:
	string text;
	Texture* textTex = nullptr;
	int textMargin;
	Alignment align;	// text alignment

public:
	Label(const Size& size = Size(), string&& line = string(), PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, Alignment alignment = Alignment::left, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	~Label() override;

	void drawSelf(const Recti& view) override;
	void onResize() override;
	void postInit() override;

	const string& getText() const;
	const Texture* getTextTex() const;
	virtual void setText(const string& str);
	virtual void setText(string&& str);
	Recti textRect() const;
	Recti textFrame() const;
	int getTextMargin() const;
	Recti texRect() const override;
protected:
	virtual int textIconOffset() const;
	virtual ivec2 textPos() const;
	virtual void updateTextTex();
	void updateTextTexNow();
};

inline const string& Label::getText() const {
	return text;
}

inline const Texture* Label::getTextTex() const {
	return textTex;
}

inline int Label::getTextMargin() const {
	return textMargin;
}

// multi-line scrollable label
class TextBox final : public Label, private Scrollable {
private:
	int lineSize;

public:
	TextBox(const Size& size = 1.f, int lineH = 30, string&& lines = string(), PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, Alignment alignment = Alignment::left, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	~TextBox() final = default;

	void tick(float dSec) final;
	void onResize() final;
	void postInit() final;
	void onHold(ivec2 mPos, uint8 mBut) final;
	void onDrag(ivec2 mPos, ivec2 mMov) final;
	void onUndrag(ivec2 mPos, uint8 but) final;
	void onScroll(ivec2 wMov) final;
	bool navSelectable() const final;

	void setText(const string& str) final;
	void setText(string&& str) final;
	Recti texRect() const final;
protected:
	int textIconOffset() const final;
	ivec2 textPos() const final;
	void updateTextTex() final;
};

// for switching between multiple options (kinda like a drop-down menu except I was too lazy to make an actual one)
class ComboBox final : public Label {
private:
	vector<string> options;
	uptr<string[]> tooltips;
	size_t curOpt;

public:
	ComboBox(const Size& size = Size(), string&& curOption = string(), vector<string>&& opts = vector<string>(), PCall call = nullptr, Texture* tip = nullptr, uptr<string[]> otips = nullptr, Alignment alignment = Alignment::left, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	ComboBox(const Size& size = Size(), size_t curOption = 0, vector<string>&& opts = vector<string>(), PCall call = nullptr, Texture* tip = nullptr, uptr<string[]> otips = nullptr, Alignment alignment = Alignment::left, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	~ComboBox() final = default;

	void onClick(ivec2 mPos, uint8 mBut) final;

	const vector<string>& getOptions() const;
	void setOptions(size_t curOption, vector<string>&& opt, uptr<string[]> tips);
	const string* getTooltips() const;
	size_t getCurOpt() const;
	void setCurOpt(size_t id);
};

inline const vector<string>& ComboBox::getOptions() const {
	return options;
}

inline const string* ComboBox::getTooltips() const {
	return tooltips.get();
}

inline size_t ComboBox::getCurOpt() const {
	return curOpt;
}

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm rather than on click)
class LabelEdit final : public Label {
public:
	enum class TextType : uint8 {
		any,
		password,
		sInt,
		sIntSpaced,
		uInt,
		uIntSpaced,
		sFloat,
		sFloatSpaced,
		uFloat,
		uFloatSpaced
	};

	static constexpr int caretWidth = 4;

	bool unfocusConfirm;
private:
	TextType textType;
	int textOfs = 0;	// text's horizontal offset
	uint cpos = 0;		// caret position
	string oldText;

public:
	LabelEdit(const Size& size = Size(), string&& line = string(), PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Texture* tip = nullptr, TextType type = TextType::any, bool focusLossConfirm = true, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	~LabelEdit() final = default;

	void drawTop(const Recti& view) const final;
	void onClick(ivec2 mPos, uint8 mBut) final;
	void onKeypress(const SDL_Keysym& key) final;
	void onCompose(string_view str, uint olen) final;
	void onText(string_view str, uint olen) final;

	const string& getOldText() const;
	void setText(const string& str) final;
	void setText(string&& str) final;

	void confirm();
	void cancel();

protected:
	ivec2 textPos() const final;
	void updateTextTex() final;
private:
	void onTextReset();
	int caretPos() const;	// caret's relative x position
	void setCPos(uint cp);

	static bool kmodCtrl(uint16 mod);
	static bool kmodAlt(uint16 mod);
	uint jumpCharB(uint i);
	uint jumpCharF(uint i);
	uint findWordStart();	// returns index of first character of word before cpos
	uint findWordEnd();		// returns index of character after last character of word after cpos
	void cleanText();
	void cleanSIntSpacedText();
	void cleanUIntSpacedText();
	void cleanSFloatText();
	void cleanSFloatSpacedText();
	void cleanUFloatText();
	void cleanUFloatSpacedText();
};

inline const string& LabelEdit::getOldText() const {
	return oldText;
}

inline bool LabelEdit::kmodCtrl(uint16 mod) {
	return mod & KMOD_CTRL && !(mod & (KMOD_SHIFT | KMOD_ALT));
}

inline bool LabelEdit::kmodAlt(uint16 mod) {
	return mod & KMOD_ALT && !(mod & (KMOD_SHIFT | KMOD_CTRL));
}

// for getting a key/button/axis
class KeyGetter final : public Label {
public:
	enum class AcceptType : uint8 {
		keyboard,
		joystick,
		gamepad
	};

	static constexpr char ellipsisStr[] = "...";
private:
	static constexpr char fmtButton[] = "B {:d}";
	static constexpr char fmtHat[] = "H {:d} {}";
	static constexpr char fmtAxis[] = "A {}{:d}";
	static constexpr char prefAxisPos = '+';
	static constexpr char prefAxisNeg = '-';

	AcceptType acceptType;		// what kind of binding is being accepted
	Binding::Type bindingType;	// index of what is currently being edited

public:
	KeyGetter(const Size& size = Size(), AcceptType type = AcceptType::keyboard, Binding::Type binding = Binding::Type(-1), Texture* tip = nullptr, Alignment alignment = Alignment::center, pair<Texture*, bool> texture = nullTex, bool bg = true, int lineMargin = defaultTextMargin, int iconMargin = defaultIconMargin);
	~KeyGetter() final = default;

	void onClick(ivec2 mPos, uint8 mBut) final;
	void onKeypress(const SDL_Keysym& key) final;
	void onJButton(uint8 jbutton) final;
	void onJHat(uint8 jhat, uint8 value) final;
	void onJAxis(uint8 jaxis, bool positive) final;
	void onGButton(SDL_GameControllerButton gbutton) final;
	void onGAxis(SDL_GameControllerAxis gaxis, bool positive) final;
	bool navSelectable() const final;

	void restoreText();
	void clearBinding();
private:
	static string bindingText(Binding::Type binding, AcceptType accept);
};

inline void KeyGetter::restoreText() {
	setText(bindingText(bindingType, acceptType));
}

// for arranging multi window setup
class WindowArranger final : public Button {
private:
	struct Dsp {
		Recti rect, full;
		Texture* txt = nullptr;
		bool active;

		Dsp() = default;
		Dsp(const Dsp&) = default;
		Dsp(Dsp&&) = default;
		Dsp(const Recti& vdsp, bool on);

		Dsp& operator=(const Dsp&) = default;
		Dsp& operator=(Dsp&&) = default;
	};

	static constexpr int winMargin = 5;

	umap<int, Dsp> disps;
	ivec2 totalDim;
	Recti dragr;
	int dragging;
	int selected;
	float bscale;
	bool vertical;

public:
	WindowArranger(const Size& size = Size(), float baseScale = 1.f, bool vertExp = true, PCall leftCall = nullptr, PCall rightCall = nullptr, Texture* tip = nullptr, bool bg = true, pair<Texture*, bool> texture = nullTex, int margin = defaultIconMargin);
	~WindowArranger() final;

	void drawSelf(const Recti& view) final;
	void drawTop(const Recti& view) const final;
	void onResize() final;
	void postInit() final;
	void onClick(ivec2 mPos, uint8 mBut) final;
	void onMouseMove(ivec2 mPos, ivec2 mMov) final;
	void onHold(ivec2 mPos, uint8 mBut) final;
	void onDrag(ivec2 mPos, ivec2 mMov) final;
	void onUndrag(ivec2 mPos, uint8 mBut) final;
	void onDisplayChange() final;
	bool navSelectable() const final;

	Color color() const final;
	const Texture* getTooltip() final;
	bool draggingDisp(int id) const;

	const umap<int, Dsp>& getDisps() const;
	umap<int, Recti> getActiveDisps() const;
	Recti offsetDisp(const Recti& rect, ivec2 pos) const;
	int precalcSizeExpand(int fsiz) const;
	tuple<Recti, Color, Recti, const Texture*> dispRect(int id, const Dsp& dsp) const;
private:
	int dispUnderPos(ivec2 pnt) const;
	void calcDisplays();
	void buildEntries();
	float entryScale(int fsiz) const;
	void freeTextures();
	ivec2 snapDrag() const;
	static array<ivec2, 8> getSnapPoints(const Recti& rect);
	template <size_t S> static void scanClosestSnapPoint(const array<pair<uint, uint>, S>& relations, const Recti& rect, const array<ivec2, 8>& snaps, uint& snapId, ivec2& snapPnt, float& minDist);
};

inline const umap<int, WindowArranger::Dsp>& WindowArranger::getDisps() const {
	return disps;
}

inline Recti WindowArranger::offsetDisp(const Recti& rect, ivec2 pos) const {
	return rect.translate(pos + winMargin);
}

inline int WindowArranger::precalcSizeExpand(int fsiz) const {
	return int(float(totalDim[vertical]) * entryScale(fsiz)) + winMargin * 2;
}
