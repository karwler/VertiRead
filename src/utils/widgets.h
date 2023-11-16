#pragma once

#include "settings.h"
#include "prog/types.h"

template <Class T>
class TextDsp {
public:
	static constexpr int textMargin = 5;

protected:
	T text;
	Texture* textTex = nullptr;

public:
	TextDsp(T&& str) : text(std::move(str)) {}
	~TextDsp();

	const T& getText() const { return text; }
	const Texture* getTextTex() const { return textTex; }
protected:
	void recreateTextTex(uint height);
	Recti dspTextFrame(const Recti& rect, const Recti& frame) const;
	ivec2 alignedTextPos(ivec2 pos, int sizx, Alignment align) const;
};

template <Class T>
Recti TextDsp<T>::dspTextFrame(const Recti& rect, const Recti& frame) const {
	return Recti(rect.x + textMargin, rect.y, rect.w - textMargin * 2, rect.h).intersect(frame);
}

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

	bool getDraggingSlider() const { return draggingSlider; }
	void setLimits(ivec2 lsize, ivec2 wsize, bool vert);
	ivec2 getListSize() const { return listSize; }
	ivec2 getListMax() const { return listMax; }
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
	Widget(const Size& size = Size()) : relSize(size) {}
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
	virtual void onHover() {}
	virtual void onUnhover() {}
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

	Layout* getParent() const { return parent; }
	void setParent(Layout* pnt, uint id);
	uint getIndex() const { return relSize.id; }
	const Size& getRelSize() const { return relSize; }
	virtual ivec2 position() const;
	virtual ivec2 size() const;
	ivec2 center() const;
	Recti rect() const;				// the rectangle that is the widget
	virtual Recti frame() const;	// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)
	virtual void setSize(const Size& size);
	int sizeToPixAbs(const Size& siz, int res) const;
};

inline ivec2 Widget::center() const {
	return position() + size() / 2;
}

inline Recti Widget::rect() const {
	return Recti(position(), size());
}

// visible widget owning a texture
class Picture final : public Widget {
private:
	Texture* tex;

public:
	Picture(const Size& size, Texture* texture);
	~Picture() override;

	void drawSelf(const Recti& view) override;

	const Texture* getTex() const { return tex; }
};

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Widget, public TextDsp<Cstring> {
public:
	const bool showBg;
	const Alignment align;

public:
	Label(const Size& size, Cstring&& line, Alignment alignment = Alignment::left, bool bg = true);
	~Label() override = default;

	void drawSelf(const Recti& view) override;
	void onResize() override;
	void postInit() override;

	virtual void setText(const Cstring& str);
	virtual void setText(Cstring&& str);
	Recti textRect() const;
	Recti textFrame() const;
protected:
	virtual ivec2 textPos() const;
	virtual void updateTextTex();
	void updateTextTexNow();
};

inline Recti Label::textFrame() const {
	return dspTextFrame(rect(), frame());
}

// multi-line scrollable label
class TextBox final : public Label, private Scrollable {
private:
	const uint lineSize;

public:
	TextBox(const Size& size, uint lineH, Cstring&& lines, bool bg = true);
	~TextBox() override = default;

	void tick(float dSec) override;
	void onResize() override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(ivec2 mPos, uint8 but) override;
	void onScroll(ivec2 wMov) override;
	bool navSelectable() const override;

	void setText(const Cstring& str) override;
	void setText(Cstring&& str) override;
protected:
	ivec2 textPos() const override;
	void updateTextTex() override;
};

// clickable widget with an event
class Button : public Widget {
protected:
	const Cstring tooltip;
	UserEvent etype;
	int16 ecode;
	Actions actions;
	Color bgColor = Color::normal;

public:
	Button(const Size& size, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring());
	~Button() override = default;

	void drawAddr(const Recti& view) override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	void onDoubleClick(ivec2 mPos, uint8 mBut) override;
	void onHover() override;
	void onUnhover() override;
	bool navSelectable() const override;
	bool hasDoubleclick() const override;

	virtual const char* getTooltip() const;
	void setEvent(EventId eid, Actions amask);
	Color getBgColor() const;
	bool toggleHighlighted();
};

inline Color Button::getBgColor() const {
	return bgColor;
}

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox final : public Button {
public:
	bool on;

	CheckBox(const Size& size, bool checked, EventId eid, Cstring&& tip = Cstring());
	~CheckBox() override = default;

	void drawSelf(const Recti& view) override;
	void onClick(ivec2 mPos, uint8 mBut) override;

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
	Slider(const Size& size, int value, int minimum, int maximum, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring());
	~Slider() override = default;

	void drawSelf(const Recti& view) override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;

	int getVal() const { return val; }
	void setVal(int value);

	Recti barRect() const;
	Recti sliderRect() const;

private:
	void setSlider(int xpos);
	int sliderPos() const;
	int sliderLim() const;
};

inline void Slider::setVal(int value) {
	val = std::clamp(value, vmin, vmax);
}

inline int Slider::sliderPos() const {
	return position().x + size().y / 4 + (val - vmin) * sliderLim() / (vmax - vmin);
}

// button with text
class PushButton : public Button, public TextDsp<Cstring> {
private:
	const Alignment align;

public:
	PushButton(const Size& size, Cstring&& line, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring(), Alignment alignment = Alignment::left);
	~PushButton() override = default;

	void drawSelf(const Recti& view) override;
	void onResize() override;
	void postInit() override;

	virtual void setText(const Cstring& str);
	virtual void setText(Cstring&& str);
	Recti textRect() const;
	Recti textFrame() const;
protected:
	virtual ivec2 textPos() const;
	virtual void updateTextTex();
	void updateTextTexNow();
};

inline Recti PushButton::textFrame() const {
	return dspTextFrame(rect(), frame());
}

// button with a texture
class IconButton final : public Button {
public:
	static constexpr int margin = 4;

private:
	const Texture* tex;

public:
	IconButton(const Size& size, const Texture* texture, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring());
	~IconButton() override = default;

	void drawSelf(const Recti& view) override;

	const Texture* getTex() const { return tex; }
	Recti texRect() const;
};

// button with text and an icon on the left
class IconPushButton final : public PushButton {
private:
	bool freeIcon;
	Texture* iconTex;

public:
	IconPushButton(const Size& size, Cstring&& line, const Texture* texture, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring());
	IconPushButton(const Size& size, Cstring&& line, Texture* texture, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring());	// gains ownership of texture
	~IconPushButton() override;

	void drawSelf(const Recti& view) override;

	const Texture* getTextTex() const { return textTex; }
	const Texture* getIconTex() const { return iconTex; }
	void setIcon(const Texture* tex);
	void setIcon(Texture* tex);	// gains ownership of texture
	Recti textRect() const;
	Recti textFrame() const;
	Recti iconRect() const;
private:
	ivec2 textPos() const override;
};

// for switching between multiple options (kinda like a drop-down menu except I was too lazy to make an actual one)
class ComboBox final : public PushButton {
private:
	uint curOpt;
	vector<Cstring> options;
	uptr<Cstring[]> tooltips;

public:
	ComboBox(const Size& size, Cstring&& curOption, vector<Cstring>&& opts, EventId call, Cstring&& tip = Cstring(), uptr<Cstring[]> otips = nullptr, Alignment alignment = Alignment::left);
	ComboBox(const Size& size, uint curOption, vector<Cstring>&& opts, EventId call, Cstring&& tip = Cstring(), uptr<Cstring[]> otips = nullptr, Alignment alignment = Alignment::left);
	~ComboBox() override = default;

	void onClick(ivec2 mPos, uint8 mBut) override;

	const vector<Cstring>& getOptions() const { return options; }
	void setOptions(uint curOption, vector<Cstring>&& opt, uptr<Cstring[]> tips);
	const Cstring* getTooltips() const { return tooltips.get(); }
	uint getCurOpt() const { return curOpt; }
	void setCurOpt(uint id);
};

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm rather than on click)
class LabelEdit final : public Button, public TextDsp<string> {
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

private:
	string oldText;
	uint cpos = 0;		// caret position
	int textOfs = 0;	// text's horizontal offset
	const TextType textType;
public:
	const bool unfocusConfirm;

	LabelEdit(const Size& size, string&& line, EventId eid, Actions amask = ACT_LEFT, Cstring&& tip = Cstring(), TextType type = TextType::any, bool focusLossConfirm = true);
	~LabelEdit() override = default;

	void drawSelf(const Recti& view) override;
	void drawTop(const Recti& view) const override;
	void onResize() override;
	void postInit() override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	void onKeypress(const SDL_Keysym& key) override;
	void onCompose(string_view str, uint olen) override;
	void onText(string_view str, uint olen) override;
	void confirm();
	void cancel();

	const string& getOldText() const { return oldText; }
	void setText(const string& str);
	void setText(string&& str);
	Recti textRect() const;
	Recti textFrame() const;

private:
	void updateTextTex();
	void updateTextTexNow();
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

inline Recti LabelEdit::textFrame() const {
	return dspTextFrame(rect(), frame());
}

inline bool LabelEdit::kmodCtrl(uint16 mod) {
	return mod & KMOD_CTRL && !(mod & (KMOD_SHIFT | KMOD_ALT));
}

inline bool LabelEdit::kmodAlt(uint16 mod) {
	return mod & KMOD_ALT && !(mod & (KMOD_SHIFT | KMOD_CTRL));
}

// for getting a key/button/axis
class KeyGetter final : public PushButton {
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

	const AcceptType acceptType;		// what kind of binding is being accepted
	const Binding::Type bindingType;	// index of what is currently being edited

public:
	KeyGetter(const Size& size, AcceptType type, Binding::Type binding, Cstring&& tip = Cstring());
	~KeyGetter() override = default;

	void onClick(ivec2 mPos, uint8 mBut) override;
	void onKeypress(const SDL_Keysym& key) override;
	void onJButton(uint8 jbutton) override;
	void onJHat(uint8 jhat, uint8 value) override;
	void onJAxis(uint8 jaxis, bool positive) override;
	void onGButton(SDL_GameControllerButton gbutton) override;
	void onGAxis(SDL_GameControllerAxis gaxis, bool positive) override;
	bool navSelectable() const override;

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
	const float bscale;
	const bool vertical;

public:
	WindowArranger(const Size& size, float baseScale, bool vertExp, EventId eid, Actions amask, Cstring&& tip = Cstring());
	~WindowArranger() override;

	void drawSelf(const Recti& view) override;
	void drawTop(const Recti& view) const override;
	void onResize() override;
	void postInit() override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	void onMouseMove(ivec2 mPos, ivec2 mMov) override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;
	void onDisplayChange() override;
	bool navSelectable() const override;

	const char* getTooltip() const override;
	bool draggingDisp(int id) const;
	const umap<int, Dsp>& getDisps() const { return disps; }
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

inline Recti WindowArranger::offsetDisp(const Recti& rect, ivec2 pos) const {
	return rect.translate(pos + winMargin);
}

inline int WindowArranger::precalcSizeExpand(int fsiz) const {
	return int(float(totalDim[vertical]) * entryScale(fsiz)) + winMargin * 2;
}
