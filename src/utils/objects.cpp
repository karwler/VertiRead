#include "engine/world.h"

EFix operator|(EFix a, EFix b) {
	return static_cast<EFix>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// OBJECT

Object::Object(const vec2i& ANC, vec2i POS, const vec2i& SIZ, EFix FIX, EColor CLR) :
	color(CLR),
	fix(FIX)
{
	// set positin to anchor if set to -1
	if (POS.x == -1)
		POS.x = ANC.x;
	if (POS.y == -1)
		POS.y = ANC.y;
	Anchor(ANC);
	Pos(POS);
	Size(SIZ);
}
Object::~Object() {}

Object* Object::Clone() const {
	return new Object(*this);
}

SDL_Rect Object::getRect() const {
	return {Pos().x, Pos().y, Size().x, Size().y};
}

vec2i Object::Anchor() const {
	vec2i ret;
	ret.x = (fix & FIX_X) ? anchor.x : pixX(anchor.x);
	ret.y = (fix & FIX_Y) ? anchor.y : pixY(anchor.y);
	return ret;
}

void Object::Anchor(const vec2i& newPos) {
	anchor.x = (fix & FIX_X) ? newPos.x : prcX(newPos.x);
	anchor.y = (fix & FIX_Y) ? newPos.y : prcY(newPos.y);
}

vec2i Object::Pos() const {
	vec2i ret = Anchor();
	ret.x = (fix & FIX_PX) ? pixX(pos.x) : (fix & FIX_W) ? ret.x + pos.x : ret.x + pixX(pos.x);
	ret.y = (fix & FIX_PY) ? pixY(pos.y) : (fix & FIX_H) ? ret.y + pos.y : ret.y + pixY(pos.y);
	return ret;
}

void Object::Pos(const vec2i& newPos) {
	vec2i dist = newPos - Anchor();
	pos.x = (fix & FIX_PX) ? prcX(newPos.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	pos.y = (fix & FIX_PY) ? prcY(newPos.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Object::End() const {
	vec2i ret = Anchor();
	ret.x = (fix & FIX_EX) ? pixX(end.x) : (fix & FIX_W) ? ret.x + end.x : ret.x + pixX(end.x);
	ret.y = (fix & FIX_EY) ? pixY(end.y) : (fix & FIX_H) ? ret.y + end.y : ret.y + pixY(end.y);
	return ret;
}

void Object::End(const vec2i& newPos) {
	vec2i dist = newPos - Anchor();
	end.x = (fix & FIX_EX) ? prcX(newPos.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	end.y = (fix & FIX_EY) ? prcY(newPos.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Object::Size() const {
	return End() - Pos();
}

void Object::Size(const vec2i& newSize) {
	vec2i dist = Pos() + newSize - Anchor();
	end.x = (fix & FIX_EX) ? prcX(Pos().x + newSize.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	end.y = (fix & FIX_EY) ? prcY(Pos().y + newSize.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

// LABEL

Label::Label(const Object& BASE, const string& TXT, ETextAlign ALG) :
	Object(BASE),
	align(ALG),
	text(TXT)
{}
Label::~Label() {}

Label* Label::Clone() const {
	return new Label(*this);
}

Text Label::getText() const {
	Text txt(text, 0, Size().y);
	txt.SetPosToRect(getRect(), align);
	return txt;
}

// BUTTON

Button::Button(const Object& BASE, void (Program::*CALLB)()) :
	Object(BASE),
	callback(CALLB)
{}
Button::~Button() {}

Button* Button::Clone() const {
	return new Button(*this);
}

void Button::OnClick() {
	if (callback)
		(World::program()->*callback)();
}

void Button::Callback(void (Program::*func)()) {
	callback = func;
}

// BUTTON TEXT

ButtonText::ButtonText(const Object& BASE, void (Program::*CALLB)(), const string& TXT, ETextAlign ALG) :
	Button(BASE, CALLB),
	align(ALG),
	text(TXT)
{}
ButtonText::~ButtonText() {}

ButtonText* ButtonText::Clone() const {
	return new ButtonText(*this);
}

Text ButtonText::getText() const {
	Text txt(text, 0, Size().y);
	txt.SetPosToRect(getRect(), align);
	return txt;
}

// BUTTON IMAGE

ButtonImage::ButtonImage(const Object& BASE, void (Program::*CALLB)(), const vector<string>& TEXS) :
	Button(BASE, CALLB),
	curTex(0)
{
	for (const string& it : TEXS)
		texes.push_back(World::library()->getTex(it));
}
ButtonImage::~ButtonImage() {}

ButtonImage* ButtonImage::Clone() const {
	return new ButtonImage(*this);
}

void ButtonImage::OnClick() {
	Button::OnClick();
	curTex++;
	if (curTex == texes.size())
		curTex = 0;
	World::engine()->SetRedrawNeeded();
}

Image ButtonImage::CurTex() const {
	return texes.empty() ? Image() : Image(Pos(), texes[curTex], Size());
}

LineEditor::LineEditor(const Object& BASE, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Object(BASE),
	LineEdit(nullptr, "", TXT, TYPE, KCALL, CCALL)
{}
LineEditor::~LineEditor() {}

LineEditor*LineEditor::Clone() const {
	return new LineEditor(*this);
}

Text LineEditor::getText() const {
	return Text(editor.Text(), Pos()-vec2i(textPos, 0), Size().y);
}

SDL_Rect LineEditor::getCaret() const {
	vec2i pos = Pos();
	int height = Size().y;

	return { Text(editor.Text().substr(0, editor.CursorPos()), 0, height).size().x - textPos + pos.x, pos.y, 5, height };
}

void LineEditor::CheckCaretRight() {
	SDL_Rect caret = getCaret();
	int diff = caret.x + caret.w - End().x;
	if (diff > 0)
		textPos += diff;
}

void LineEditor::CheckCaretLeft() {
	SDL_Rect caret = getCaret();
	int diff = Pos().x - caret.x;
	if (diff > 0)
		textPos -= diff;
}
