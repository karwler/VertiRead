#include "engine/world.h"

// FIX

EFix operator~(EFix a) {
	return static_cast<EFix>(~static_cast<uint8>(a));
}
EFix operator&(EFix a, EFix b) {
	return static_cast<EFix>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EFix operator&=(EFix& a, EFix b) {
	return a = static_cast<EFix>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EFix operator^(EFix a, EFix b) {
	return static_cast<EFix>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EFix operator^=(EFix& a, EFix b) {
	return a = static_cast<EFix>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EFix operator|(EFix a, EFix b) {
	return static_cast<EFix>(static_cast<uint8>(a) | static_cast<uint8>(b));
}
EFix operator|=(EFix& a, EFix b) {
	return a = static_cast<EFix>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// widget

Widget::Widget(const vec2i& ANC, vec2i POS, const vec2i& SIZ, EFix FIX, EColor CLR) :
	color(CLR),
	fix(FIX)
{
	// set pos to anchor if less than 0
	if (POS.x < 0)
		POS.x = ANC.x;
	if (POS.y < 0)
		POS.y = ANC.y;

	// set values according to fix
	setAnchor(ANC);
	setPos(POS);
	setSize(SIZ);
}
Widget::~Widget() {}

Widget* Widget::clone() const {
	return new Widget(*this);
}

SDL_Rect Widget::rect() const {
	vec2i ps = pos();
	vec2i sz = size();
	return {ps.x, ps.y, sz.x, sz.y};
}

vec2i Widget::anchor() const {
	vec2i ret;
	ret.x = (fix & FIX_X) ? vanc.x : pixX(vanc.x);
	ret.y = (fix & FIX_Y) ? vanc.y : pixY(vanc.y);
	return ret;
}

void Widget::setAnchor(const vec2i& newPos) {
	vanc.x = (fix & FIX_X) ? newPos.x : prcX(newPos.x);
	vanc.y = (fix & FIX_Y) ? newPos.y : prcY(newPos.y);
}

vec2i Widget::pos() const {
	vec2i ret = anchor();
	ret.x = (fix & FIX_PX) ? pixX(vpos.x) : (fix & FIX_W) ? ret.x + vpos.x : ret.x + pixX(vpos.x);
	ret.y = (fix & FIX_PY) ? pixY(vpos.y) : (fix & FIX_H) ? ret.y + vpos.y : ret.y + pixY(vpos.y);
	return ret;
}

void Widget::setPos(const vec2i& newPos) {
	vec2i dist = newPos - anchor();
	vpos.x = (fix & FIX_PX) ? prcX(newPos.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	vpos.y = (fix & FIX_PY) ? prcY(newPos.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Widget::end() const {
	vec2i ret = anchor();
	ret.x = (fix & FIX_EX) ? pixX(vend.x) : (fix & FIX_W) ? ret.x + vend.x : ret.x + pixX(vend.x);
	ret.y = (fix & FIX_EY) ? pixY(vend.y) : (fix & FIX_H) ? ret.y + vend.y : ret.y + pixY(vend.y);
	return ret;
}

void Widget::setEnd(const vec2i& newPos) {
	vec2i dist = newPos - anchor();
	vend.x = (fix & FIX_EX) ? prcX(newPos.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	vend.y = (fix & FIX_EY) ? prcY(newPos.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Widget::size() const {
	return end() - pos();
}

void Widget::setSize(const vec2i& newSize) {
	vec2i dist = pos() + newSize - anchor();
	vend.x = (fix & FIX_EX) ? prcX(pos().x + newSize.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	vend.y = (fix & FIX_EY) ? prcY(pos().y + newSize.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

// LABEL

Label::Label(const Widget& BASE, const string& LBL, ETextAlign ALG) :
	Widget(BASE),
	align(ALG),
	label(LBL)
{}
Label::~Label() {}

Label* Label::clone() const {
	return new Label(*this);
}

Text Label::text() const {
	Text txt(label, 0, size().y);
	txt.setPosToRect(rect(), align);
	return txt;
}

// BUTTON

Button::Button(const Widget& BASE, void (Program::*CALLB)()) :
	Widget(BASE),
	call(CALLB)
{}
Button::~Button() {}

Button* Button::clone() const {
	return new Button(*this);
}

void Button::onClick(ClickType click) {
	if (call && click.button == SDL_BUTTON_LEFT)
		(World::program()->*call)();
}

void Button::setCall(void (Program::*func)()) {
	call = func;
}

// BUTTON TEXT

ButtonText::ButtonText(const Widget& BASE, void (Program::*CALLB)(), const string& LBL, ETextAlign ALG) :
	Button(BASE, CALLB),
	align(ALG),
	label(LBL)
{}
ButtonText::~ButtonText() {}

ButtonText* ButtonText::clone() const {
	return new ButtonText(*this);
}

Text ButtonText::text() const {
	Text txt(label, 0, size().y);
	txt.setPosToRect(rect(), align);
	return txt;
}

// BUTTON IMAGE

ButtonImage::ButtonImage(const Widget& BASE, void (Program::*CALLB)(), const vector<string>& TEXS) :
	Button(BASE, CALLB),
	curTex(0)
{
	texes.resize(TEXS.size());
	for (size_t i=0; i!=TEXS.size(); i++)
		texes[i] = World::library()->texture(TEXS[i]);
}
ButtonImage::~ButtonImage() {}

ButtonImage* ButtonImage::clone() const {
	return new ButtonImage(*this);
}

void ButtonImage::onClick(ClickType click) {
	Button::onClick(click);
	if (click.button == SDL_BUTTON_LEFT) {
		curTex++;
		if (curTex == texes.size())
			curTex = 0;

		World::winSys()->setRedrawNeeded();
	}
}

Image ButtonImage::getCurTex() const {
	return texes.empty() ? Image() : Image(pos(), texes[curTex], size());
}

// LINE EDITOR

LineEditor::LineEditor(const Widget& BASE, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Widget(BASE),
	LineEdit(nullptr, "", TXT, TYPE, KCALL, CCALL)
{}
LineEditor::~LineEditor() {}

LineEditor*LineEditor::clone() const {
	return new LineEditor(*this);
}

Text LineEditor::text() const {
	return Text(editor.getText(), pos()-vec2i(textPos, 0), size().y);
}

SDL_Rect LineEditor::caretRect() const {
	vec2i ps = pos();
	int height = size().y;

	return {Text(editor.getText().substr(0, editor.getCaretPos()), 0, height).size().x - textPos + ps.x, ps.y, 5, height};
}

void LineEditor::checkCaretRight() {
	SDL_Rect caret = caretRect();
	int diff = caret.x + caret.w - end().x;
	if (diff > 0)
		textPos += diff;
}

void LineEditor::checkCaretLeft() {
	SDL_Rect caret = caretRect();
	int diff = pos().x - caret.x;
	if (diff > 0)
		textPos -= diff;
}

// POPUP

Popup::Popup(const Widget& BASE, const vector<Widget*>& wgtS) :
	Widget(BASE),
	widgets(wgtS)
{}

Popup::~Popup() {
	clear(widgets);
}

Popup*Popup::clone() const {
	return new Popup(*this);
}
