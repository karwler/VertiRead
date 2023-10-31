#include "layouts.h"
#include "engine/drawSys.h"
#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/world.h"
#include "prog/program.h"
#include "prog/progs.h"

// SCROLL BAR

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

bool Scrollable::hold(ivec2 mPos, uint8 mBut, Widget* wgt, ivec2 pos, ivec2 size, bool vert) {
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

void Scrollable::drag(ivec2 mPos, ivec2 mMov, ivec2 pos, bool vert) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse, pos, vert);
	else
		moveListPos(mMov * vswap(0, -1, !vert));
}

void Scrollable::undrag(ivec2 mPos, uint8 mBut, bool vert) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mPos, mBut) && !draggingSlider)
			motion = World::inputSys()->getMouseMove() * vswap(0, -1, !vert);
		SDL_CaptureMouse(SDL_FALSE);
		World::scene()->setCapture(nullptr);	// should call cancelDrag through the captured widget
	}
}

void Scrollable::scroll(ivec2 wMov, bool vert) {
	moveListPos(vswap(wMov.x, wMov.y, !vert));
	motion = vec2(0.f);
}

void Scrollable::setLimits(ivec2 lsize, ivec2 wsize, bool vert) {
	listSize = lsize;
	listMax = ivec2(wsize.x < lsize.x ? lsize.x - wsize.x : 0, wsize.y < listSize.y ? lsize.y - wsize.y : 0);
	sliderSize = wsize[vert] < lsize[vert] ? wsize[vert] * wsize[vert] / lsize[vert] : wsize[vert];
	sliderMax = wsize[vert] - sliderSize;
}

void Scrollable::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		if (mov -= throttle * dSec; mov < 0.f)
			mov = 0.f;
	} else if (mov += throttle * dSec; mov > 0.f)
		mov = 0.f;
}

void Scrollable::setSlider(int spos, ivec2 pos, bool vert) {
	int lim = listMax[vert];
	listPos[vert] = std::clamp((spos - pos[vert]) * lim / sliderMax, 0, lim);
}

Recti Scrollable::barRect(ivec2 pos, ivec2 size, bool vert) const {
	int bs = barSize(size, vert);
	return vert ? Rect(pos.x + size.x - bs, pos.y, bs, size.y) : Rect(pos.x, pos.y + size.y - bs, size.x, bs);
}

Recti Scrollable::sliderRect(ivec2 pos, ivec2 size, bool vert) const {
	int bs = barSize(size, vert);
	int sp = sliderPos(pos, size, vert);
	return vert ? Rect(pos.x + size.x - bs, sp, bs, sliderSize) : Rect(sp, pos.y + size.y - bs, sliderSize, bs);
}

// WIDGET

bool Widget::navSelectable() const {
	return false;
}

bool Widget::hasDoubleclick() const {
	return false;
}

void Widget::setParent(Layout* pnt, uint id) {
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

Picture::Picture(const Size& size, bool bg, pair<Texture*, bool> texture, int margin) :
	Widget(size),
	tex(texture.first),
	freeTex(texture.second),
	showBG(bg),
	texMargin(margin)
{}

Picture::~Picture() {
	if (freeTex)
		World::drawSys()->getRenderer()->freeTexture(tex);
}

void Picture::drawSelf(const Recti& view) {
	World::drawSys()->drawPicture(this, view);
}

void Picture::drawAddr(const Recti& view) {
	World::drawSys()->drawPictureAddr(this, view);
}

Color Picture::color() const {
	return Color::normal;
}

Recti Picture::texRect() const {
	Recti rct = rect();
	return Recti(rct.pos() + texMargin, rct.size() - texMargin * 2);
}

void Picture::setTex(Texture* texture, bool exclusive) {
	if (freeTex)
		World::drawSys()->getRenderer()->freeTexture(tex);
	tex = texture;
	freeTex = tex && exclusive;
}

// BUTTON

Button::Button(const Size& size, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, bool bg, pair<Texture*, bool> texture, int margin) :
	Picture(size, bg, texture, margin),
	lcall(leftCall),
	rcall(rightCall),
	dcall(doubleCall),
	tooltip(tip)
{}

Button::~Button() {
	if (tooltip)
		World::drawSys()->getRenderer()->freeTexture(tooltip);
}

void Button::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (ScrollArea* sa = dynamic_cast<ScrollArea*>(parent))
			sa->selectWidget(relSize.id);
		World::program()->exec(lcall, this);
	} else if (mBut == SDL_BUTTON_RIGHT) {
		if (ScrollArea* sa = dynamic_cast<ScrollArea*>(parent))
			sa->deselectWidget(relSize.id);
		World::program()->exec(rcall, this);
	}
}

void Button::onDoubleClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::program()->exec(dcall, this);
}

bool Button::navSelectable() const {
	return lcall || rcall || dcall || tooltip;
}

bool Button::hasDoubleclick() const {
	return dcall;
}

Color Button::color() const {
	if (ScrollArea* sa = dynamic_cast<ScrollArea*>(parent); sa && sa->getSelected().contains(const_cast<Button*>(this)))
		return Color::light;
	if (navSelectable() && World::scene()->select == this)
		return Color::select;
	return Color::normal;
}

const Texture* Button::getTooltip() {
	return tooltip;
}

Recti Button::tooltipRect() const {
	ivec2 view = World::drawSys()->getViewRes();
	Recti rct(World::winSys()->mousePos() + ivec2(0, DrawSys::cursorHeight), tooltip ? tooltip->getRes() + tooltipMargin * 2 : ivec2(0));
	if (rct.x + rct.w > view.x)
		rct.x = view.x - rct.w;
	if (rct.y + rct.h > view.y)
		rct.y = rct.y - DrawSys::cursorHeight - rct.h;
	return rct;
}

void Button::setCalls(optional<PCall> lc, optional<PCall> rc, optional<PCall> dc) {
	if (lc)
		lcall = *lc;
	if (rc)
		rcall = *rc;
	if (dc)
		dcall = *dc;
}

// CHECK BOX

CheckBox::CheckBox(const Size& size, bool checked, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, bool bg, pair<Texture*, bool> texture, int margin) :
	Button(size, leftCall, rightCall, doubleCall, tip, bg, texture, margin),
	on(checked)
{}

void CheckBox::drawSelf(const Recti& view) {
	World::drawSys()->drawCheckBox(this, view);
}

void CheckBox::onClick(ivec2 mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		toggle();
	Button::onClick(mPos, mBut);
}

Recti CheckBox::boxRect() const {
	ivec2 siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Recti(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& size, int value, int minimum, int maximum, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, bool bg, pair<Texture*, bool> texture, int margin) :
	Button(size, leftCall, rightCall, doubleCall, tip, bg, texture, margin),
	val(value),
	vmin(minimum),
	vmax(maximum)
{}

void Slider::drawSelf(const Recti& view) {
	World::drawSys()->drawSlider(this, view);
}

void Slider::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::program()->exec(rcall, this);
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

void Slider::setSlider(int xpos) {
	setVal((xpos - position().x - size().y/4) * vmax / sliderLim());
	World::program()->exec(lcall, this);
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
	return siz.x - siz.y/2 - Scrollable::barSizeVal;
}

// PROGRESS BAR

ProgressBar::ProgressBar(const Size& size, int value, int minimum, int maximum, bool bg, pair<Texture*, bool> texture, int margin) :
	Picture(size, bg, texture, margin),
	val(value),
	vmin(minimum),
	vmax(maximum)
{}

void ProgressBar::drawSelf(const Recti& view) {
	World::drawSys()->drawProgressBar(this, view);
}

Recti ProgressBar::barRect() const {
	ivec2 siz = size();
	int margin = siz.y / barMarginFactor;
	return Recti(position() + margin, ivec2(val * (siz.x - margin * 2) / (vmax - vmin), siz.y - margin * 2));
}

// LABEL

Label::Label(const Size& size, string&& line, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, Alignment alignment, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Button(size, leftCall, rightCall, doubleCall, tip, bg, texture, iconMargin),
	text(std::move(line)),
	textMargin(lineMargin),
	align(alignment)
{}

Label::~Label() {
	if (textTex)
		World::drawSys()->getRenderer()->freeTexture(textTex);
}

void Label::drawSelf(const Recti& view) {
	World::drawSys()->drawLabel(this, view);
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(const string& str) {
	text = str;
	updateTextTexNow();
}

void Label::setText(string&& str) {
	text = std::move(str);
	updateTextTexNow();
}

Recti Label::textRect() const {
	return Recti(textPos(), textTex->getRes());
}

Recti Label::textFrame() const {
	Recti rct = rect();
	int ofs = textIconOffset();
	return Recti(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).intersect(frame());
}

Recti Label::texRect() const {
	Recti rct = rect();
	vec2 res = getTex()->getRes();
	ivec2 siz = res * float(rct.h - texMargin * 2) / std::max(res.x, res.y);
	return Recti(rct.pos() + (rct.h - siz) / 2, siz);
}

int Label::textIconOffset() const {
	return getTex() ? size().y : 0;
}

ivec2 Label::textPos() const {
	ivec2 pos = position();
	switch (align) {
	using enum Alignment;
	case left:
		return ivec2(pos.x + textIconOffset() + textMargin, pos.y);
	case center: {
		int iofs = textIconOffset();
		return ivec2(pos.x + iofs + (size().x - iofs - textTex->getRes().x) / 2, pos.y); }
	case right:
		return ivec2(pos.x + size().x - textTex->getRes().x - textMargin, pos.y);
	}
	return pos;
}

void Label::updateTextTex() {
	if (textTex)
		World::drawSys()->getRenderer()->freeTexture(textTex);
	textTex = World::drawSys()->renderText(text, size().y);
}

void Label::updateTextTexNow() {
	updateTextTex();
	World::drawSys()->getRenderer()->synchTransfer();
}

// TEXT BOX

TextBox::TextBox(const Size& size, int lineH, string&& lines, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, Alignment alignment, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Label(size, std::move(lines), leftCall, rightCall, doubleCall, tip, alignment, texture, bg, lineMargin, iconMargin),
	lineSize(lineH)
{}

void TextBox::tick(float dSec) {
	Scrollable::tick(dSec);
}

void TextBox::onResize() {
	updateTextTex();
	setListPos(listPos);
}

void TextBox::postInit() {
	updateTextTex();
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

bool TextBox::navSelectable() const {
	return true;
}

Recti TextBox::texRect() const {
	return Picture::texRect();
}

int TextBox::textIconOffset() const {
	return 0;
}

ivec2 TextBox::textPos() const {
	return Label::textPos() - listPos;
}

void TextBox::setText(const string& str) {
	text = str;
	listPos = ivec2(0);
}

void TextBox::setText(string&& str) {
	text = std::move(str);
	listPos = ivec2(0);
}

void TextBox::updateTextTex() {
	if (textTex)
		World::drawSys()->getRenderer()->freeTexture(textTex);
	if (textTex = World::drawSys()->renderText(text, lineSize, size().x); textTex)
		setLimits(textTex->getRes(), size(), true);
}

// COMBO BOX

ComboBox::ComboBox(const Size& size, string&& curOption, vector<string>&& opts, PCall call, Texture* tip, uptr<string[]> otips, Alignment alignment, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Label(size, std::move(curOption), call, call, nullptr, tip, alignment, texture, bg, lineMargin, iconMargin),
	options(std::move(opts)),
	tooltips(std::move(otips)),
	curOpt(std::min(size_t(rng::find(options, text) - options.begin()), options.size()))
{}

ComboBox::ComboBox(const Size& size, size_t curOption, vector<string>&& opts, PCall call, Texture* tip, uptr<string[]> otips, Alignment alignment, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Label(size, valcp(opts[curOption]), call, call, nullptr, tip, alignment, texture, bg, lineMargin, iconMargin),
	options(std::move(opts)),
	tooltips(std::move(otips)),
	curOpt(curOption)
{}

void ComboBox::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		World::scene()->setContext(World::program()->getState()->createComboContext(this, mBut == SDL_BUTTON_LEFT ? lcall : rcall));
}

void ComboBox::setOptions(size_t curOption, vector<string>&& opts, uptr<string[]> tips) {
	setText(opts[curOption]);
	curOpt = curOption;
	options = std::move(opts);
	tooltips = std::move(tips);
}

void ComboBox::setCurOpt(size_t id) {
	curOpt = std::min(id, options.size());
	setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(const Size& size, string&& line, PCall leftCall, PCall rightCall, PCall doubleCall, Texture* tip, TextType type, bool focusLossConfirm, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Label(size, std::move(line), leftCall, rightCall, doubleCall, tip, Alignment::left, texture, bg, lineMargin, iconMargin),
	unfocusConfirm(focusLossConfirm),
	textType(type),
	oldText(text)
{
	cleanText();
}

void LabelEdit::drawTop(const Recti& view) const {
	ivec2 ps = position();
	World::drawSys()->drawCaret(Recti(caretPos() + ps.x + textIconOffset() + textMargin, ps.y, caretWidth, size().y), frame(), view);
}

void LabelEdit::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		Recti rct = rect();
		World::scene()->setCapture(this);
		SDL_StartTextInput();
		SDL_SetTextInputRect(reinterpret_cast<SDL_Rect*>(&rct));
		setCPos(text.length());
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::program()->exec(rcall, this);
}

void LabelEdit::onKeypress(const SDL_Keysym& key) {
	switch (key.scancode) {
	case SDL_SCANCODE_LEFT:	// move caret left
		if (kmodAlt(key.mod))	// if holding alt skip word
			setCPos(findWordStart());
		else if (kmodCtrl(key.mod))	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos > 0)	// otherwise go left by one
			setCPos(jumpCharB(cpos));
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (kmodAlt(key.mod))	// if holding alt skip word
			setCPos(findWordEnd());
		else if (kmodCtrl(key.mod))	// if holding ctrl go to end
			setCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(jumpCharF(cpos));
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (kmodAlt(key.mod)) {	// if holding alt delete left word
			uint id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTexNow();
			setCPos(id);
		} else if (kmodCtrl(key.mod)) {	// if holding ctrl delete line to left
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
		if (kmodAlt(key.mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTexNow();
		} else if (kmodCtrl(key.mod)) {	// if holding ctrl delete line to right
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
		if (kmodCtrl(key.mod))
			if (char* ctxt = SDL_GetClipboardText()) {
				uint garbagio = 0;
				onText(ctxt, garbagio);
				SDL_free(ctxt);
			}
		break;
	case SDL_SCANCODE_C:	// copy text
		if (kmodCtrl(key.mod))
			SDL_SetClipboardText(text.c_str());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (kmodCtrl(key.mod)) {
			SDL_SetClipboardText(text.c_str());
			setText(string());
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (kmodCtrl(key.mod))
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

ivec2 LabelEdit::textPos() const {
	ivec2 pos = position();
	return ivec2(pos.x + textOfs + textIconOffset() + textMargin, pos.y);
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
		Label::updateTextTex();
	else {
		if (textTex)
			World::drawSys()->getRenderer()->freeTexture(textTex);
		uint cnt = 0;
		for (uint i = 0; i < text.length(); i = jumpCharF(i), ++cnt);
		textTex = World::drawSys()->renderText(string(cnt, '*'), size().y);
	}
}

void LabelEdit::setCPos(uint cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - texMargin*2; ce > sx)
		textOfs -= ce - sx;
}

int LabelEdit::caretPos() const {
	return World::drawSys()->textLength(string_view(text.c_str(), cpos), size().y) + textOfs;
}

void LabelEdit::confirm() {
	textOfs = 0;
	World::scene()->setCapture(nullptr);
	SDL_StopTextInput();
	World::program()->exec(lcall, this);
}

void LabelEdit::cancel() {
	textOfs = 0;
	text = oldText;
	updateTextTexNow();

	World::scene()->setCapture(nullptr);
	SDL_StopTextInput();
}

uint LabelEdit::jumpCharB(uint i) {
	while (--i && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::jumpCharF(uint i) {
	while (++i < text.length() && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::findWordStart() {
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

uint LabelEdit::findWordEnd() {
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
		text.erase(rng::remove_if(text, [](char c) -> bool { return !isdigit(c); }).begin(), text.end());
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
		else if (*it == '.'  && !dot) {
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
		else if (*it == '.'  && !dot) {
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

KeyGetter::KeyGetter(const Size& size, AcceptType type, Binding::Type binding, Texture* tip, Alignment alignment, pair<Texture*, bool> texture, bool bg, int lineMargin, int iconMargin) :
	Label(size, bindingText(binding, type), nullptr, nullptr, nullptr, tip, alignment, texture, bg, lineMargin, iconMargin),
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

void KeyGetter::onKeypress(const SDL_Keysym& key) {
	if (acceptType == AcceptType::keyboard) {
		World::inputSys()->getBinding(bindingType).setKey(key.scancode);
		setText(SDL_GetScancodeName(key.scancode));
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
		setText(std::format(fmtHat, jhat, Binding::hatNames.at(value)));
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

bool KeyGetter::navSelectable() const {
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
			return std::format(fmtHat, bind.getJctID(), Binding::hatNames.at(bind.getJhatVal()));
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

WindowArranger::Dsp::Dsp(const Recti& vdsp, bool on) :
	full(vdsp),
	active(on)
{}

WindowArranger::WindowArranger(const Size& size, float baseScale, bool vertExp, PCall leftCall, PCall rightCall, Texture* tip, bool bg, pair<Texture*, bool> texture, int margin) :
	Button(size, leftCall, rightCall, nullptr, tip, bg, texture, margin),
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
		if (dsp.txt)
			World::drawSys()->getRenderer()->freeTexture(dsp.txt);
}

void WindowArranger::calcDisplays() {
	umap<int, Recti> all = Settings::displayArrangement();
	disps.reserve(all.size());
	totalDim = ivec2(0);

	for (auto& [id, rect] : World::sets()->displays) {
		disps.try_emplace(id, rect, true);
		totalDim = glm::max(totalDim, rect.end());
	}
	ivec2 border = vswap(totalDim[vertical], 0, vertical);
	for (auto& [id, rect] : all) {
		Recti dst = rect;
		if (rng::any_of(disps, [&dst](const pair<int, Dsp>& p) -> bool { return p.second.full.overlaps(dst); })) {
			dst.pos() = border;
			border[vertical] += dst.size()[vertical];
		}
		if (auto [it, ok] = disps.try_emplace(id, dst, false); ok)
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
	World::drawSys()->drawWaDisp(dragr, Color::light, dsp.txt ? Recti(dragr.pos() + (dragr.size() - dsp.txt->getRes()) / 2, dsp.txt->getRes()) : Recti(0), dsp.txt, frame(), view);
}

void WindowArranger::onResize() {
	freeTextures();
	buildEntries();
}

void WindowArranger::postInit() {
	buildEntries();
}

void WindowArranger::onClick(ivec2 mPos, uint8 mBut) {
	if (((mBut == SDL_BUTTON_LEFT && lcall) || (mBut == SDL_BUTTON_RIGHT && rcall)) && disps.size() > 1) {
		selected = dispUnderPos(mPos);
		if (umap<int, Dsp>::iterator it = disps.find(selected); it != disps.end()) {
			it->second.active = !it->second.active;
			World::program()->exec(mBut == SDL_BUTTON_LEFT ? lcall : rcall, this);
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
		dsp.full.pos() = spos.x >= 0 && spos.y >= 0 ? spos : ivec2(vec2(dragr.pos() - position() - winMargin) / scale);
		glm::clamp(dsp.full.pos(), ivec2(0), totalDim);
		totalDim = glm::max(totalDim, dsp.full.end());
		dsp.rect.pos() = vec2(dsp.full.pos()) * scale;
		World::program()->exec(lcall, this);
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

array<ivec2, 8> WindowArranger::getSnapPoints(const Recti& rect) {
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

bool WindowArranger::navSelectable() const {
	return true;
}

Color WindowArranger::color() const {
	return Color::dark;
}

const Texture* WindowArranger::getTooltip() {
	return disps.contains(selected) ? tooltip : nullptr;
}

bool WindowArranger::draggingDisp(int id) const {
	return id == dragging && World::scene()->getCapture() == this;
}

int WindowArranger::dispUnderPos(ivec2 pnt) const {
	ivec2 pos = position();
	for (const auto& [id, dsp] : disps)
		if (offsetDisp(dsp.rect, pos).contains(pnt))
			return id;
	return Renderer::singleDspId;
}

float WindowArranger::entryScale(int fsiz) const {
	fsiz -= winMargin * 2;
	return int(float(totalDim[!vertical]) * bscale) <= fsiz ? bscale : float(totalDim[!vertical]) / float(fsiz);
}

tuple<Recti, Color, Recti, const Texture*> WindowArranger::dispRect(int id, const Dsp& dsp) const {
	ivec2 offs = position() + winMargin;
	Recti rct = dsp.rect.translate(offs);
	Color clr = id != selected || World::scene()->select != this || World::scene()->getCapture() != this ? dsp.active ? Color::light : Color::normal : Color::select;
	return tuple(rct, clr, dsp.txt ? Recti(rct.pos() + (rct.size() - dsp.txt->getRes()) / 2, dsp.txt->getRes()) : Recti(0), dsp.txt);
}

umap<int, Recti> WindowArranger::getActiveDisps() const {
	umap<int, Recti> act;
	for (auto& [id, dsp] : disps)
		if (dsp.active)
			act.emplace(id, dsp.full);
	return act;
}
