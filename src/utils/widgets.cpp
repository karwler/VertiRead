#include "layouts.h"
#include "engine/drawSys.h"
#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/world.h"
#include "prog/program.h"
#include "prog/progs.h"
#include <SDL_clipboard.h>
#include <cfloat>
#include <format>

template <Class T>
TextDsp<T>::~TextDsp() {
	World::drawSys()->getRenderer()->freeTexture(textTex);
}

template <Class T>
void TextDsp<T>::recreateTextTex(string_view str, uint height) {
	if (textTex) {
		if (!World::drawSys()->renderText(textTex, str, height)) {
			World::drawSys()->getRenderer()->freeTexture(textTex);
			textTex = nullptr;
		}
	} else
		textTex = World::drawSys()->renderText(str, height);
}

template <Class T>
void TextDsp<T>::recreateTextTex(string_view str, uint height, uint limit) {
	if (textTex) {
		if (!World::drawSys()->renderText(textTex, str, height, limit)) {
			World::drawSys()->getRenderer()->freeTexture(textTex);
			textTex = nullptr;
		}
	} else
		textTex = World::drawSys()->renderText(str, height, limit);
}

template <Class T>
ivec2 TextDsp<T>::alignedTextPos(ivec2 pos, int sizx, Alignment align) const noexcept {
	switch (align) {
	using enum Alignment;
	case left:
		return ivec2(pos.x + textMargin, pos.y);
	case center:
		return ivec2(pos.x + (sizx - textTex->getRes().x) / 2, pos.y);
	case right:
		return ivec2(pos.x + sizx - textTex->getRes().x - textMargin, pos.y);
	}
	return pos;
}

// SCROLLABLE

bool Scrollable::tick(float dSec) {
	if (motion != vec2(0.f)) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
		World::scene()->updateSelect();
		return true;
	}
	return false;
}

bool Scrollable::hold(ivec2 mPos, uint8 mBut, Widget* wgt, ivec2 pos, ivec2 size, bool vert) noexcept {
	bool moved = false;
	motion = vec2(0.f);	// get rid of scroll motion
	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->setCapture(wgt);
		SDL_CaptureMouse(SDL_TRUE);
		if ((draggingSlider = barRect(pos, size, vert).contains(mPos))) {
			int sp = sliderPos(pos, size, vert), ss = sliderSize;
			if (moved = outRange(mPos[vert], sp, sp + ss); moved)	// if mouse outside of slider but inside bar
				setSlider(mPos[vert] - ss / 2, pos, vert);
			diffSliderMouse = mPos.y - sliderPos(pos, size, vert);	// get difference between mouse y and slider y
		}
	}
	return moved;
}

void Scrollable::drag(ivec2 mPos, ivec2 mMov, ivec2 pos, bool vert) noexcept {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse, pos, vert);
	else
		moveListPos(mMov * vswap(0, -1, !vert));
}

void Scrollable::undrag(ivec2 mPos, uint8 mBut, bool vert) noexcept {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mPos, mBut) && !draggingSlider)
			motion = World::inputSys()->getMouseMove() * vswap(0, -1, !vert);
		SDL_CaptureMouse(SDL_FALSE);
		World::scene()->setCapture(nullptr);	// should call cancelDrag through the captured widget
	}
}

void Scrollable::scroll(ivec2 wMov, bool vert) noexcept {
	moveListPos(vswap(wMov.x, wMov.y, !vert));
	motion = vec2(0.f);
}

void Scrollable::setLimits(ivec2 lsize, ivec2 wsize, bool vert) noexcept {
	listSize = lsize;
	listMax = ivec2(wsize.x < lsize.x ? lsize.x - wsize.x : 0, wsize.y < listSize.y ? lsize.y - wsize.y : 0);
	sliderSize = wsize[vert] < lsize[vert] ? wsize[vert] * wsize[vert] / lsize[vert] : wsize[vert];
	sliderMax = wsize[vert] - sliderSize;
}

void Scrollable::throttleMotion(float& mov, float dSec) noexcept {
	if (mov > 0.f) {
		if (mov -= throttle * dSec; mov < 0.f)
			mov = 0.f;
	} else if (mov += throttle * dSec; mov > 0.f)
		mov = 0.f;
}

void Scrollable::setSlider(int spos, ivec2 pos, bool vert) noexcept {
	int lim = listMax[vert];
	listPos[vert] = std::clamp((spos - pos[vert]) * lim / sliderMax, 0, lim);
}

Recti Scrollable::barRect(ivec2 pos, ivec2 size, bool vert) const noexcept {
	int bs = barSize(size, vert);
	return vert ? Recti(pos.x + size.x - bs, pos.y, bs, size.y) : Recti(pos.x, pos.y + size.y - bs, size.x, bs);
}

Recti Scrollable::sliderRect(ivec2 pos, ivec2 size, bool vert) const noexcept {
	int bs = barSize(size, vert);
	int sp = sliderPos(pos, size, vert);
	return vert ? Recti(pos.x + size.x - bs, sp, bs, sliderSize) : Recti(sp, pos.y + size.y - bs, sliderSize, bs);
}

// WIDGET

bool Widget::navSelectable() const noexcept {
	return false;
}

bool Widget::hasDoubleclick() const noexcept {
	return false;
}

void Widget::setParent(Layout* pnt, uint id) noexcept {
	parent = pnt;
	relSize.id = id;
}

ivec2 Widget::position() const {
	return parent->wgtPosition(relSize.id);
}

ivec2 Widget::size() const {
	return parent->wgtSize(relSize.id);
}

Recti Widget::frame() const {
	return parent->frame();
}

void Widget::setSize(const Size& size) {
	relSize = size;
	parent->onResize();
	World::drawSys()->getRenderer()->synchTransfer();
}

void Widget::onNavSelect(Direction dir) {
	parent->navSelectNext(relSize.id, dir.vertical() ? center().x : center().y, dir);
}

int Widget::sizeToPixAbs(const Size& siz, int res) const {
	switch (siz.mod) {
	using enum Size::Mode;
	case rela:
		return int(siz.prc * float(res));
	case pixv:
		return siz.pix;
	case calc:
		return siz(this);
	}
	return 0;
}

// PICTURE

Picture::Picture(const Size& size, Texture* texture) noexcept :
	Widget(size),
	tex(texture)
{}

Picture::~Picture() {
	World::drawSys()->getRenderer()->freeTexture(tex);
}

void Picture::drawSelf(const Recti& view) {
	World::drawSys()->drawPicture(this, view);
}

// LABEL

Label::Label(const Size& size, Cstring&& line, Alignment alignment, bool bg) noexcept :
	Widget(size),
	TextDsp(std::move(line)),
	showBg(bg),
	align(alignment)
{}

void Label::drawSelf(const Recti& view) {
	World::drawSys()->drawLabel(this, view);
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(const Cstring& str) {
	text = str;
	updateTextTexNow();
}

void Label::setText(Cstring&& str) {
	text = std::move(str);
	updateTextTexNow();
}

Recti Label::textRect() const {
	return Recti(textPos(), textTex->getRes());
}

ivec2 Label::textPos() const {
	return alignedTextPos(position(), size().x, align);
}

void Label::updateTextTex() {
	recreateTextTex(text.data(), size().y);
}

void Label::updateTextTexNow() {
	updateTextTex();
	World::drawSys()->getRenderer()->synchTransfer();
}

// TEXT BOX

TextBox::TextBox(const Size& size, uint lineH, Cstring&& lines, bool bg) noexcept :
	Label(size, std::move(lines), Alignment::left, bg),
	lineSize(lineH)
{}

void TextBox::tick(float dSec) {
	Scrollable::tick(dSec);
}

void TextBox::onResize() {
	updateTextTex();
	setListPos(listPos);
}

void TextBox::onHold(ivec2 mPos, uint8 mBut) {
	hold(mPos, mBut, this, position(), size(), true);
}

void TextBox::onDrag(ivec2 mPos, ivec2 mMov) {
	drag(mPos, mMov, position(), true);
}

void TextBox::onUndrag(ivec2 mPos, uint8 mBut) {
	undrag(mPos, mBut, true);
}

void TextBox::onScroll(ivec2 wMov) {
	scroll(wMov, true);
}

bool TextBox::navSelectable() const noexcept {
	return true;
}

ivec2 TextBox::textPos() const {
	return Label::textPos() - listPos;
}

void TextBox::setText(const Cstring& str) {
	Label::setText(str);
	listPos = ivec2(0);
}

void TextBox::setText(Cstring&& str) {
	Label::setText(std::move(str));
	listPos = ivec2(0);
}

void TextBox::updateTextTex() {
	recreateTextTex(text.data(), lineSize, size().x);
	setLimits(textTex ? textTex->getRes() : uvec2(0), size(), true);
}

// BUTTON

Button::Button(const Size& size, EventId eid, Actions amask, Cstring&& tip) noexcept :
	Widget(size),
	tooltip(std::move(tip)),
	etype(eid.type),
	ecode(eid.code),
	actions(amask)
{}

void Button::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (actions & ACT_LEFT))
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
	else if (mBut == SDL_BUTTON_RIGHT && (actions & ACT_RIGHT))
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_RIGHT)));
}

void Button::onDoubleClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (actions & ACT_DOUBLE))
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_DOUBLE)));
}

void Button::onHover() {
	if (bgColor == Color::normal)
		bgColor = Color::select;
}

void Button::onUnhover() {
	if (bgColor == Color::select)
		bgColor = Color::normal;
}

bool Button::toggleHighlighted() noexcept {
	bool off = bgColor != Color::light;
	bgColor = off ? Color::light : navSelectable() && World::scene()->getSelect() == this ? Color::select : Color::normal;
	return !off;
}

bool Button::navSelectable() const noexcept {
	return (etype && actions) || tooltip.filled();
}

bool Button::hasDoubleclick() const noexcept {
	return actions & ACT_DOUBLE;
}

const char* Button::getTooltip() const {
	return tooltip.filled() ? tooltip.data() : nullptr;
}

void Button::setEvent(EventId eid, Actions amask) noexcept {
	etype = eid.type;
	ecode = eid.code;
	actions = amask;
}

// CHECK BOX

CheckBox::CheckBox(const Size& size, bool checked, EventId eid, Cstring&& tip) noexcept :
	Button(size, eid, ACT_LEFT, std::move(tip)),
	on(checked)
{}

void CheckBox::drawSelf(const Recti& view) {
	World::drawSys()->drawCheckBox(this, view);
}

void CheckBox::onClick(ivec2 mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		toggle();
	Button::onClick(mPos, mBut);
}

Recti CheckBox::boxRect() const {
	ivec2 siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Recti(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& size, int value, int minimum, int maximum, EventId eid, Actions amask, Cstring&& tip) noexcept :
	Button(size, eid, amask, std::move(tip)),
	val(value),
	vmin(minimum),
	vmax(maximum)
{}

void Slider::drawSelf(const Recti& view) {
	World::drawSys()->drawSlider(this, view);
}

void Slider::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT && (actions & ACT_RIGHT))
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_RIGHT)));
}

void Slider::onHold(ivec2 mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->setCapture(this);
		SDL_CaptureMouse(SDL_TRUE);
		if (int sp = sliderPos(); outRange(mPos.x, sp, sp + Scrollable::barSizeVal))	// if mouse outside of slider
			setSlider(mPos.x - Scrollable::barSizeVal / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(ivec2 mPos, ivec2) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {	// if dragging slider stop dragging slider
		SDL_CaptureMouse(SDL_FALSE);
		World::scene()->setCapture(nullptr);
	}
}

void Slider::onKeypress(SDL_Scancode key, SDL_Keymod) {
	switch (key) {
	case SDL_SCANCODE_RIGHT:
		if (val < vmax) {
			setVal(val + 1);
			if (actions & ACT_LEFT)
				pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
		}
		break;
	case SDL_SCANCODE_LEFT:
		if (val > vmin) {
			setVal(val - 1);
			if (actions & ACT_LEFT)
				pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
		}
		break;
	case SDL_SCANCODE_DOWN: case SDL_SCANCODE_END:
		if (val != vmax) {
			val = vmax;
			if (actions & ACT_LEFT)
				pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
		}
		break;
	case SDL_SCANCODE_UP: case SDL_SCANCODE_HOME:
		if (val != vmin) {
			val = vmin;
			if (actions & ACT_LEFT)
				pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
		}
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_ESCAPE:
		onUndrag(ivec2(), SDL_BUTTON_LEFT);
	}
}

void Slider::setSlider(int xpos) {
	setVal((xpos - position().x - size().y / 4) * (vmax - vmin) / sliderLim() + vmin);
	if (actions & ACT_LEFT)
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
}

Recti Slider::barRect() const {
	ivec2 siz = size();
	int height = siz.y / 2;
	return Recti(position() + siz.y / 4, ivec2(siz.x - height, height));
}

Recti Slider::sliderRect() const {
	ivec2 pos = position(), siz = size();
	return Recti(sliderPos(), pos.y, Scrollable::barSizeVal, siz.y);
}

int Slider::sliderLim() const {
	ivec2 siz = size();
	return siz.x - siz.y / 2 - Scrollable::barSizeVal;
}

// PUSH BUTTON

PushButton::PushButton(const Size& size, Cstring&& line, EventId eid, Actions amask, Cstring&& tip, Alignment alignment) noexcept :
	Button(size, eid, amask, std::move(tip)),
	TextDsp(std::move(line)),
	align(alignment)
{}

void PushButton::drawSelf(const Recti& view) {
	World::drawSys()->drawPushButton(this, view);
}

void PushButton::onResize() {
	updateTextTex();
}

void PushButton::postInit() {
	updateTextTex();
}

void PushButton::setText(const Cstring& str) {
	text = str;
	updateTextTexNow();
}

void PushButton::setText(Cstring&& str) {
	text = std::move(str);
	updateTextTexNow();
}

Recti PushButton::textRect() const {
	return Recti(textPos(), textTex->getRes());
}

ivec2 PushButton::textPos() const {
	return alignedTextPos(position(), size().x, align);
}

void PushButton::updateTextTex() {
	recreateTextTex(text.data(), size().y);
}

void PushButton::updateTextTexNow() {
	updateTextTex();
	World::drawSys()->getRenderer()->synchTransfer();
}

// ICON BUTTON

IconButton::IconButton(const Size& size, const Texture* texture, EventId eid, Actions amask, Cstring&& tip) noexcept :
	Button(size, eid, amask, std::move(tip)),
	tex(texture)
{}

void IconButton::drawSelf(const Recti& view) {
	World::drawSys()->drawIconButton(this, view);
}

Recti IconButton::texRect() const {
	Recti rct = rect();
	return Recti(rct.pos() + margin, rct.size() - margin * 2);
}

// ICON PUSH BUTTON

IconPushButton::IconPushButton(const Size& size, Cstring&& line, const Texture* texture, EventId eid, Actions amask, Cstring&& tip) noexcept :
	PushButton(size, std::move(line), eid, amask, std::move(tip), Alignment::left),
	freeIcon(false),
	iconTex(const_cast<Texture*>(texture))
{}

IconPushButton::IconPushButton(const Size& size, Cstring&& line, Texture* texture, EventId eid, Actions amask, Cstring&& tip) noexcept :
	PushButton(size, std::move(line), eid, amask, std::move(tip), Alignment::left),
	freeIcon(true),
	iconTex(texture)
{}

IconPushButton::~IconPushButton() {
	if (freeIcon)
		World::drawSys()->getRenderer()->freeTexture(iconTex);
}

void IconPushButton::drawSelf(const Recti& view) {
	World::drawSys()->drawIconPushButton(this, view);
}

Recti IconPushButton::textRect() const {
	return Recti(textPos(), textTex->getRes());
}

Recti IconPushButton::textFrame() const {
	Recti rct = rect();
	int ofs = iconTex ? rct.h : 0;
	return Recti(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).intersect(frame());
}

ivec2 IconPushButton::textPos() const {
	ivec2 pos = position();
	int ofs = iconTex ? size().y : 0;
	return ivec2(pos.x + ofs + textMargin, pos.y);
}

Recti IconPushButton::iconRect() const {
	Recti rct = rect();
	vec2 res = iconTex->getRes();
	ivec2 siz = res * float(rct.h - IconButton::margin * 2) / std::max(res.x, res.y);
	return Recti(rct.pos() + (rct.h - siz) / 2, siz);
}

void IconPushButton::setIcon(const Texture* tex) {
	if (freeIcon)
		World::drawSys()->getRenderer()->freeTexture(iconTex);
	iconTex = const_cast<Texture*>(tex);
	freeIcon = false;
}

void IconPushButton::setIcon(Texture* tex) {
	if (freeIcon)
		World::drawSys()->getRenderer()->freeTexture(iconTex);
	iconTex = tex;
	freeIcon = true;
}

// COMBO BOX

ComboBox::ComboBox(const Size& size, Cstring&& curOption, vector<Cstring>&& opts, EventId call, Cstring&& tip, uptr<Cstring[]> otips, Alignment alignment) :
	PushButton(size, std::move(curOption), call, ACT_LEFT | ACT_RIGHT, std::move(tip), alignment),
	curOpt(std::min(size_t(rng::find(opts, text) - opts.begin()), opts.size())),
	options(std::move(opts)),
	tooltips(std::move(otips))
{}

ComboBox::ComboBox(const Size& size, uint curOption, vector<Cstring>&& opts, EventId call, Cstring&& tip, uptr<Cstring[]> otips, Alignment alignment) :
	PushButton(size, valcp(opts[curOption]), call, ACT_LEFT | ACT_RIGHT, std::move(tip), alignment),
	curOpt(curOption),
	options(std::move(opts)),
	tooltips(std::move(otips))
{}

void ComboBox::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		World::program()->getState()->showComboContext(this, EventId(etype, ecode));
}

void ComboBox::setOptions(uint curOption, vector<Cstring>&& opts, uptr<Cstring[]> tips) {
	setText(opts[curOption]);
	curOpt = curOption;
	options = std::move(opts);
	tooltips = std::move(tips);
}

void ComboBox::setCurOpt(uint id) {
	curOpt = std::min(id, uint(options.size()));
	setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(const Size& size, string&& line, EventId eid, EventId cid, Actions amask, Cstring&& tip, TextType type, bool focusLossConfirm) noexcept :
	Button(size, eid, amask, std::move(tip)),
	TextDsp(std::move(line)),
	oldText(text),
	cancEtype(cid.type),
	cancEcode(cid.code),
	textType(type),
	unfocusConfirm(focusLossConfirm)
{
	cleanText();
}

void LabelEdit::drawSelf(const Recti& view) {
	World::drawSys()->drawLabelEdit(this, view);
}

void LabelEdit::drawTop(const Recti& view) const {
	ivec2 ps = position();
	World::drawSys()->drawCaret(Recti(caretPos() + ps.x + textMargin, ps.y, caretWidth, size().y), frame(), view);
}

void LabelEdit::onResize() {
	updateTextTex();
}

void LabelEdit::postInit() {
	updateTextTex();
}

void LabelEdit::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->setCapture(this);
#if SDL_VERSION_ATLEAST(3, 0, 0)
		SDL_StartTextInput(World::scene()->getCaptureWindow());
#else
		Recti rct = rect();
		SDL_StartTextInput();
		SDL_SetTextInputRect(&rct.asRect());
#endif
		setCPos(text.length());
	} else if (mBut == SDL_BUTTON_RIGHT && (actions & ACT_RIGHT))
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_RIGHT)));
}

void LabelEdit::onKeypress(SDL_Scancode key, SDL_Keymod mod) {
	switch (key) {
	case SDL_SCANCODE_LEFT:	// move caret left
		if (kmodAlt(mod))	// if holding alt skip word
			setCPos(findWordStart());
		else if (kmodCtrl(mod))	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos > 0)	// otherwise go left by one
			setCPos(jumpCharB(cpos));
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (kmodAlt(mod))	// if holding alt skip word
			setCPos(findWordEnd());
		else if (kmodCtrl(mod))	// if holding ctrl go to end
			setCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(jumpCharF(cpos));
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (kmodAlt(mod)) {	// if holding alt delete left word
			uint id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTexNow();
			setCPos(id);
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTextTexNow();
			setCPos(0);
		} else if (cpos > 0) {	// otherwise delete left character
			uint id = jumpCharB(cpos);
			text.erase(id, cpos - id);
			updateTextTexNow();
			setCPos(id);
		}
		break;
	case SDL_SCANCODE_DELETE:	// delete right character
		if (kmodAlt(mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTexNow();
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTexNow();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, jumpCharF(cpos) - cpos);
			updateTextTexNow();
		}
		break;
	case SDL_SCANCODE_HOME:	// move caret to beginning
		setCPos(0);
		break;
	case SDL_SCANCODE_END:	// move caret to end
		setCPos(text.length());
		break;
	case SDL_SCANCODE_V:	// paste text
		if (kmodCtrl(mod))
			if (char* ctxt = SDL_GetClipboardText()) {
				uint garbagio = 0;
				onText(ctxt, garbagio);
				SDL_free(ctxt);
			}
		break;
	case SDL_SCANCODE_C:	// copy text
		if (kmodCtrl(mod))
			SDL_SetClipboardText(text.data());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (kmodCtrl(mod)) {
			SDL_SetClipboardText(text.data());
			setText(string());
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (kmodCtrl(mod))
			setText(std::move(oldText));
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		confirm();
		break;
	case SDL_SCANCODE_ESCAPE:
		cancel();
	}
}

void LabelEdit::onCompose(string_view str, uint olen) {
	text.erase(cpos, olen);
	text.insert(cpos, str);
	updateTextTexNow();
}

void LabelEdit::onText(string_view str, uint olen) {
	text.erase(cpos, olen);
	text.insert(cpos, str.data(), str.length());
	cleanText();
	updateTextTexNow();
	setCPos(cpos + str.length());
}

void LabelEdit::setText(const string& str) {
	oldText = std::move(text);
	text = str;
	onTextReset();
}

void LabelEdit::setText(string&& str) {
	oldText = std::move(text);
	text = std::move(str);
	onTextReset();
}

void LabelEdit::onTextReset() {
	cleanText();
	updateTextTexNow();
	setCPos(text.length());
}

void LabelEdit::updateTextTex() {
	if (textType != TextType::password)
		recreateTextTex(text, size().y);
	else {
		uint cnt = 0;
		for (uint i = 0; i < text.length(); i = jumpCharF(i), ++cnt);
		recreateTextTex(string(cnt, '*'), size().y);
	}
}

void LabelEdit::updateTextTexNow() {
	updateTextTex();
	World::drawSys()->getRenderer()->synchTransfer();
}

Recti LabelEdit::textRect() const {
	ivec2 pos = position();
	return Recti(pos.x + textOfs + textMargin, pos.y, textTex->getRes());
}

void LabelEdit::setCPos(uint cp) {
	cpos = cp;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	int cl = caretPos();
	if (cl < 0) {
		textOfs -= cl;
		cl = 0;
	} else if (int ce = cl + caretWidth, sx = size().x; ce > sx) {
		int diff = ce - sx;
		textOfs -= diff;
		cl -= diff;
	}
	Recti rct = rect();
	SDL_SetTextInputArea(World::scene()->getCaptureWindow(), &rct.asRect(), cl);
#else
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x; ce > sx)
		textOfs -= ce - sx;
#endif
}

int LabelEdit::caretPos() const {
	if (textType != TextType::password)
		return World::drawSys()->textLength(string_view(text.data(), cpos), size().y) + textOfs;

	uint cnt = 0;
	for (uint i = 0; i < cpos; i = jumpCharF(i), ++cnt);
	return World::drawSys()->textLength(string(cnt, '*'), size().y) + textOfs;
}

void LabelEdit::confirm() {
	textOfs = 0;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_StopTextInput(World::scene()->getCaptureWindow());
#else
	SDL_StopTextInput();
#endif
	World::scene()->setCapture(nullptr);
	if (actions & ACT_LEFT)
		pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
}

void LabelEdit::cancel() {
	textOfs = 0;
	text = oldText;
	updateTextTexNow();

#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_StopTextInput(World::scene()->getCaptureWindow());
#else
	SDL_StopTextInput();
#endif
	World::scene()->setCapture(nullptr);
	if (actions & ACT_LEFT)
		pushEvent(EventId(cancEtype, cancEcode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
}

uint LabelEdit::jumpCharB(uint i) const noexcept {
	while (--i && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::jumpCharF(uint i) const noexcept {
	while (++i < text.length() && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::findWordStart() const noexcept {
	uint i = cpos;
	if (i == text.length() && i)
		--i;
	else if (notSpace(text[i]) && i)
		if (uint n = jumpCharB(i); isSpace(text[n]))	// skip if first letter of word
			i = n;
	for (; isSpace(text[i]) && i; --i);		// skip first spaces
	for (; notSpace(text[i]) && i; --i);	// skip word
	return i ? i + 1 : i;			// correct position if necessary
}

uint LabelEdit::findWordEnd() const noexcept {
	uint i = cpos;
	for (; isSpace(text[i]) && i < text.length(); ++i);		// skip first spaces
	for (; notSpace(text[i]) && i < text.length(); ++i);	// skip word
	return i;
}

void LabelEdit::cleanText() {
	switch (textType) {
	using enum TextType;
	case sInt:
		text.erase(std::remove_if(text.begin() + ptrdiff_t(text[0] == '-'), text.end(), [](char c) -> bool { return !isdigit(c); }), text.end());
		break;
	case sIntSpaced:
		cleanSIntSpacedText();
		break;
	case uInt:
		text.erase(std::remove_if(text.begin(), text.end(), [](char c) -> bool { return !isdigit(c); }), text.end());
		break;
	case uIntSpaced:
		cleanUIntSpacedText();
		break;
	case sFloat:
		cleanSFloatText();
		break;
	case sFloatSpaced:
		cleanSFloatSpacedText();
		break;
	case uFloat:
		cleanUFloatText();
		break;
	case uFloatSpaced:
		cleanUFloatSpacedText();
	}
}

void LabelEdit::cleanSIntSpacedText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	for (string::iterator it = text.begin() + ptrdiff_t(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				++it;
		} else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isdigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUIntSpacedText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isdigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + ptrdiff_t(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == '.' && !dot) {
			dot = true;
			++it;
		} else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatSpacedText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + ptrdiff_t(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				++it;
		} else if (*it == '.' && !dot) {
			dot = true;
			++it;
		} else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == '.' && !dot) {
			dot = true;
			++it;
		} else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatSpacedText() {
	text.erase(text.begin(), rng::find_if(text, notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else if (*it == '.' && !dot) {
			dot = true;
			++it;
		} else {
			ptrdiff_t ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

// KEY GETTER

KeyGetter::KeyGetter(const Size& size, AcceptType type, Binding::Type binding, Cstring&& tip) noexcept :
	PushButton(size, bindingText(binding, type), nullEvent, ACT_NONE, std::move(tip), Alignment::center),
	acceptType(type),
	bindingType(binding)
{}

void KeyGetter::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->setCapture(this);
		setText(ellipsisStr);
	} else if (mBut == SDL_BUTTON_RIGHT) {
		clearBinding();
		World::scene()->setCapture(nullptr);
	}
}

void KeyGetter::onKeypress(SDL_Scancode key, SDL_Keymod) {
	if (acceptType == AcceptType::keyboard) {
		World::inputSys()->getBinding(bindingType).setKey(key);
		setText(SDL_GetScancodeName(key));
	}
	World::scene()->setCapture(nullptr);
}

void KeyGetter::onJButton(uint8 jbutton) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJbutton(jbutton);
		setText(std::format(fmtButton, jbutton));
	}
	World::scene()->setCapture(nullptr);
}

void KeyGetter::onJHat(uint8 jhat, uint8 value) {
	if (acceptType == AcceptType::joystick) {
		if (value != SDL_HAT_UP && value != SDL_HAT_RIGHT && value != SDL_HAT_DOWN && value != SDL_HAT_LEFT) {
			if (value & SDL_HAT_RIGHT)
				value = SDL_HAT_RIGHT;
			else if (value & SDL_HAT_LEFT)
				value = SDL_HAT_LEFT;
		}
		World::inputSys()->getBinding(bindingType).setJhat(jhat, value);
		setText(std::format(fmtHat, jhat, Binding::hatValueToName(value)));
	}
	World::scene()->setCapture(nullptr);
}

void KeyGetter::onJAxis(uint8 jaxis, bool positive) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJaxis(jaxis, positive);
		setText(std::format(fmtAxis, positive ? prefAxisPos : prefAxisNeg, jaxis));
	}
	World::scene()->setCapture(nullptr);
}

void KeyGetter::onGButton(SDL_GameControllerButton gbutton) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGbutton(gbutton);
		setText(Binding::gbuttonNames[eint(gbutton)]);
	}
	World::scene()->setCapture(nullptr);
}

void KeyGetter::onGAxis(SDL_GameControllerAxis gaxis, bool positive) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGaxis(gaxis, positive);
		setText(std::format("{}{}", positive ? prefAxisPos : prefAxisNeg, Binding::gaxisNames[eint(gaxis)]));
	}
	World::scene()->setCapture(nullptr);
}

bool KeyGetter::navSelectable() const noexcept {
	return true;
}

void KeyGetter::clearBinding() {
	switch (acceptType) {
	using enum AcceptType;
	case keyboard:
		World::inputSys()->getBinding(bindingType).clearAsgKey();
		break;
	case joystick:
		World::inputSys()->getBinding(bindingType).clearAsgJct();
		break;
	case gamepad:
		World::inputSys()->getBinding(bindingType).clearAsgGct();
	}
	setText(string());
}

string KeyGetter::bindingText(Binding::Type binding, KeyGetter::AcceptType accept) {
	switch (const Binding& bind = World::inputSys()->getBinding(binding); accept) {
	using enum AcceptType;
	case keyboard:
		if (bind.keyAssigned())
			return SDL_GetScancodeName(bind.getKey());
		break;
	case joystick:
		if (bind.jbuttonAssigned())
			return std::format(fmtButton, bind.getJctID());
		else if (bind.jhatAssigned())
			return std::format(fmtHat, bind.getJctID(), Binding::hatValueToName(bind.getJhatVal()));
		else if (bind.jaxisAssigned())
			return std::format(fmtAxis, bind.jposAxisAssigned() ? prefAxisPos : prefAxisNeg, bind.getJctID());
		break;
	case gamepad:
		if (bind.gbuttonAssigned())
			return Binding::gbuttonNames[eint(bind.getGbutton())];
		else if (bind.gaxisAssigned())
			return std::format("{}{}", bind.gposAxisAssigned() ? prefAxisPos : prefAxisNeg, Binding::gaxisNames[eint(bind.getGaxis())]);
	}
	return string();
}

// WINDOW ARRANGER

WindowArranger::Dsp::Dsp(const Recti& vdsp, bool on) noexcept :
	full(vdsp),
	active(on)
{}

WindowArranger::WindowArranger(const Size& size, float baseScale, bool vertExp, EventId eid, Actions amask, Cstring&& tip) noexcept :
	Button(size, eid, amask, std::move(tip)),
	bscale(baseScale),
	vertical(vertExp)
{
	calcDisplays();
}

WindowArranger::~WindowArranger() {
	freeTextures();
}

void WindowArranger::freeTextures() {
	for (auto& [id, dsp] : disps)
		World::drawSys()->getRenderer()->freeTexture(dsp.txt);
}

void WindowArranger::calcDisplays() {
	vector<Settings::Display> all = Settings::displayArrangement();
	disps.reserve(all.size());
	totalDim = ivec2(0);

	for (const Settings::Display& dit : World::sets()->displays) {
		disps.try_emplace(dit.did, dit.rect, true);
		totalDim = glm::max(totalDim, dit.rect.end());
	}
	ivec2 border = vswap(totalDim[vertical], 0, vertical);
	for (const Settings::Display& dit : all) {
		Recti dst = dit.rect;
		if (rng::any_of(disps, [&dst](const pair<const int, Dsp>& p) -> bool { return p.second.full.overlaps(dst); })) {
			dst.pos() = border;
			border[vertical] += dst.size()[vertical];
		}
		if (auto [it, ok] = disps.try_emplace(dit.did, dst, false); ok)
			totalDim = glm::max(totalDim, dst.end());
	}
}

void WindowArranger::buildEntries() {
	float scale = entryScale(size()[!vertical]);
	for (auto& [id, dsp] : disps) {
		dsp.rect = Recti(vec2(dsp.full.pos()) * scale, vec2(dsp.full.size()) * scale);
		dsp.txt = World::drawSys()->renderText(toStr(id), dsp.rect.h);
	}
}

void WindowArranger::drawSelf(const Recti& view) {
	World::drawSys()->drawWindowArranger(this, view);
}

void WindowArranger::drawTop(const Recti& view) const {
	const Dsp& dsp = disps.at(dragging);
	World::drawSys()->drawWaDisp(dragr, Color::light, dsp.txt ? Recti(dragr.pos() + (dragr.size() - ivec2(dsp.txt->getRes())) / 2, dsp.txt->getRes()) : Recti(0), dsp.txt, frame(), view);
}

void WindowArranger::onResize() {
	freeTextures();
	buildEntries();
}

void WindowArranger::postInit() {
	buildEntries();
}

void WindowArranger::onClick(ivec2 mPos, uint8 mBut) {
	if (((mBut == SDL_BUTTON_LEFT && (actions & ACT_LEFT)) || (mBut == SDL_BUTTON_RIGHT && (actions & ACT_RIGHT))) && disps.size() > 1) {
		selected = dispUnderPos(mPos);
		if (umap<int, Dsp>::iterator it = disps.find(selected); it != disps.end()) {
			it->second.active = !it->second.active;
			pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(mBut == SDL_BUTTON_LEFT ? ACT_LEFT : ACT_RIGHT)));
		}
	}
}

void WindowArranger::onMouseMove(ivec2 mPos, ivec2) {
	selected = dispUnderPos(mPos);
}

void WindowArranger::onHold(ivec2 mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && disps.size() > 1) {
		dragging = dispUnderPos(mPos);
		if (umap<int, Dsp>::iterator it = disps.find(dragging); it != disps.end()) {
			World::scene()->setCapture(this);
			SDL_CaptureMouse(SDL_TRUE);
			dragr = it->second.rect.translate(position() + winMargin);
		}
	}
}

void WindowArranger::onDrag(ivec2 mPos, ivec2) {
	dragr.pos() = mPos - disps.at(dragging).rect.size() / 2;
	if (ivec2 spos = snapDrag(); spos.x >= 0 && spos.y >= 0)
		dragr.pos() = ivec2(vec2(spos) * entryScale(size()[!vertical])) + position() + winMargin;
}

void WindowArranger::onUndrag(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		SDL_CaptureMouse(SDL_FALSE);
		World::scene()->setCapture(nullptr);

		ivec2 spos = snapDrag();
		float scale = entryScale(size()[!vertical]);
		Dsp& dsp = disps.find(dragging)->second;
		dsp.full.pos() = glm::clamp(spos.x >= 0 && spos.y >= 0 ? spos : ivec2(vec2(dragr.pos() - position() - winMargin) / scale), ivec2(0), totalDim);
		totalDim = glm::max(totalDim, dsp.full.end());
		dsp.rect.pos() = vec2(dsp.full.pos()) * scale;
		if (actions & ACT_LEFT)
			pushEvent(EventId(etype, ecode), this, std::bit_cast<void*>(uintptr_t(ACT_LEFT)));
	}
}

ivec2 WindowArranger::snapDrag() const {
	static constexpr array<pair<uint, uint>, 24> snapRelationsOuter = {
		pair(0, 2), pair(0, 5), pair(0, 7),	// top left to top right, bottom left, bottom right
		pair(1, 5), pair(1, 6), pair(1, 7),	// top center to bottom left, bottom center, bottom right
		pair(2, 0), pair(2, 5), pair(2, 7),	// top right to top left, bottom left, bottom right
		pair(3, 2), pair(3, 4), pair(3, 7),	// left center to right top, right center, right bottom
		pair(4, 0), pair(4, 3), pair(4, 5),	// right center to left top, left center, left bottom
		pair(5, 0), pair(5, 2), pair(5, 7),	// bottom left to top left, top right, bottom right
		pair(6, 0), pair(6, 1), pair(6, 2),	// bottom center to top left, top center, top right
		pair(7, 0), pair(7, 2), pair(7, 5)	// bottom right to top left, top right, bottom left
	};
	static constexpr array<pair<uint, uint>, 8> snapRelationsInner = {
		pair(0, 0), pair(1, 1), pair(2, 2), pair(3, 3), pair(4, 4), pair(5, 5), pair(6, 6), pair(7, 7)
	};
	static constexpr ivec2 (* const offsetCalc[8])(ivec2, ivec2) = {
		[](ivec2, ivec2 pnt) -> ivec2 { return pnt; },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x - siz.x / 2, pnt.y); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x - siz.x, pnt.y); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x, pnt.y - siz.y / 2); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x - siz.x, pnt.y - siz.y / 2); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x, pnt.y - siz.y); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return ivec2(pnt.x - siz.x / 2, pnt.y - siz.y); },
		[](ivec2 siz, ivec2 pnt) -> ivec2 { return pnt - siz; }
	};

	umap<int, Dsp>::const_iterator snapFrom = disps.find(dragging);
	array<ivec2, 8> snaps = getSnapPoints(Recti(vec2(dragr.pos() - position() - winMargin) / entryScale(size()[!vertical]), snapFrom->second.full.size()));
	uint snapId;
	ivec2 snapPnt;
	float minDist = FLT_MAX;
	for (umap<int, Dsp>::const_iterator it = disps.begin(); it != disps.end(); ++it)
		if (it->first != dragging)
			scanClosestSnapPoint(snapRelationsOuter, it->second.full, snaps, snapId, snapPnt, minDist);
	scanClosestSnapPoint(snapRelationsInner, Recti(ivec2(0), totalDim), snaps, snapId, snapPnt, minDist);
	return minDist <= float(std::min(snapFrom->second.full.w, snapFrom->second.full.h)) / 3.f
		? offsetCalc[snapId](snapFrom->second.full.size(), snapPnt)
		: ivec2(INT_MIN);
}

array<ivec2, 8> WindowArranger::getSnapPoints(const Recti& rect) noexcept {
	return {
		rect.pos(),
		ivec2(rect.x + rect.w / 2, rect.y),
		ivec2(rect.end().x, rect.y),
		ivec2(rect.x, rect.y + rect.h / 2),
		ivec2(rect.end().x, rect.y + rect.h / 2),
		ivec2(rect.x, rect.end().y),
		ivec2(rect.x + rect.w / 2, rect.end().y),
		rect.end()
	};
}

template <size_t S>
void WindowArranger::scanClosestSnapPoint(const array<pair<uint, uint>, S>& relations, const Recti& rect, const array<ivec2, 8>& snaps, uint& snapId, ivec2& snapPnt, float& minDist) {
	array<ivec2, 8> rpnts = getSnapPoints(rect);
	for (auto [from, to] : relations)
		if (float dist = glm::length(vec2(rpnts[to] - snaps[from])); dist < minDist) {
			minDist = dist;
			snapPnt = rpnts[to];
			snapId = from;
		}
}

void WindowArranger::onDisplayChange() {
	freeTextures();
	disps.clear();
	calcDisplays();
	buildEntries();
}

bool WindowArranger::navSelectable() const noexcept {
	return true;
}

const char* WindowArranger::getTooltip() const {
	return tooltip.filled() && disps.contains(selected) ? tooltip.data() : nullptr;
}

bool WindowArranger::draggingDisp(int id) const noexcept {
	return id == dragging && World::scene()->getCapture() == this;
}

int WindowArranger::dispUnderPos(ivec2 pnt) const {
	ivec2 pos = position();
	for (const auto& [id, dsp] : disps)
		if (offsetDisp(dsp.rect, pos).contains(pnt))
			return id;
	return Renderer::singleDspId;
}

float WindowArranger::entryScale(int fsiz) const noexcept {
	fsiz -= winMargin * 2;
	return int(float(totalDim[!vertical]) * bscale) <= fsiz ? bscale : float(totalDim[!vertical]) / float(fsiz);
}

WindowArranger::DspDisp WindowArranger::dispRect(int id, const Dsp& dsp) const {
	ivec2 offs = position() + winMargin;
	Recti rct = dsp.rect.translate(offs);
	return {
		.rect = rct,
		.text = dsp.txt ? Recti(rct.pos() + (rct.size() - ivec2(dsp.txt->getRes())) / 2, dsp.txt->getRes()) : Recti(0),
		.tex = dsp.txt,
		.color = id != selected || World::scene()->getSelect() != this || World::scene()->getCapture() != this ? dsp.active ? Color::light : Color::normal : Color::select
	};
}

vector<Settings::Display> WindowArranger::getActiveDisps() const {
	vector<Settings::Display> act;
	for (auto& [id, dsp] : disps)
		if (dsp.active)
			act.emplace_back(dsp.full, id);
	std::sort(act.begin(), act.end());
	return act;
}
