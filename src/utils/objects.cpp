#include "engine/world.h"

EFix operator|(EFix a, EFix b) {
	return static_cast<EFix>(static_cast<byte>(a) | static_cast<byte>(b));
}

// OBJECT

Object::Object(vec2i ANC, vec2i POS, vec2i SIZ, EFix FIX, EColor CLR) :
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

SDL_Rect Object::getRect() const {
	return {Pos().x, Pos().y, Size().x, Size().y};
}

vec2i Object::Anchor() const {
	vec2i ret;
	ret.x = (fix & FIX_X) ? anchor.x : pixX(anchor.x);
	ret.y = (fix & FIX_Y) ? anchor.y : pixY(anchor.y);
	return ret;
}

void Object::Anchor(vec2i newPos) {
	anchor.x = (fix & FIX_X) ? newPos.x : prcX(newPos.x);
	anchor.y = (fix & FIX_Y) ? newPos.y : prcY(newPos.y);
}

vec2i Object::Pos() const {
	vec2i ret = Anchor();
	ret.x += (fix & FIX_W) ? pos.x : pixX(pos.x);
	ret.y += (fix & FIX_H) ? pos.y : pixY(pos.y);
	return ret;
}

void Object::Pos(vec2i newPos) {
	vec2i dist = newPos - Anchor();
	pos.x = (fix & FIX_W) ? dist.x : prcX(dist.x);
	pos.y = (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Object::End() const {
	vec2i ret = Anchor();
	ret.x = (fix & FIX_EX) ? pixX(end.x) : (fix & FIX_W) ? ret.x + end.x : ret.x + pixX(end.x);
	ret.y = (fix & FIX_EY) ? pixY(end.y) : (fix & FIX_H) ? ret.y + end.y : ret.y + pixY(end.y);
	return ret;
}

void Object::End(vec2i newPos) {
	vec2i dist = newPos - Anchor();
	end.x = (fix & FIX_EX) ? prcX(newPos.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	end.y = (fix & FIX_EY) ? prcY(newPos.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

vec2i Object::Size() const {
	return End() - Pos();
}

void Object::Size(vec2i newSize) {
	vec2i dist = Pos() + newSize - Anchor();
	end.x = (fix & FIX_EX) ? prcX(Pos().x + newSize.x) : (fix & FIX_W) ? dist.x : prcX(dist.x);
	end.y = (fix & FIX_EY) ? prcY(Pos().y + newSize.y) : (fix & FIX_H) ? dist.y : prcY(dist.y);
}

// LABEL

Label::Label(const Object& BASE, string TXT) :
	Object(BASE),
	text(TXT)
{}
Label::~Label() {}

Text Label::getText() const {
	return Text(text, Pos()+vec2i(5, 0), Size().y, 8);
}

// BUTTON

Button::Button(const Object& BASE, void (Program::*CALLB)()) :
	Object(BASE),
	callback(CALLB)
{}
Button::~Button() {}

void Button::OnClick() {
	if (callback)
		(World::program()->*callback)();
}

void Button::Callback(void (Program::*func)()) {
	callback = func;
}

// BUTTON IMAGE

ButtonImage::ButtonImage(const Object& BASE, void (Program::*CALLB)(), const vector<string>& TEXS) :
	Object(BASE),
	curTex(0)
{
	callback = CALLB;

	for (const string& it : TEXS)
		texes.push_back(World::library()->getTex(it));
}
ButtonImage::~ButtonImage() {}

void ButtonImage::OnClick() {
	Button::OnClick();
	curTex++;
	if (curTex == texes.size())
		curTex = 0;
	World::engine->SetRedrawNeeded();
}

Image ButtonImage::CurTex() const {
	return texes.empty() ? Image() : Image(Pos(), texes[curTex], Size());
}

// BUTTON TEXT

ButtonText::ButtonText(const Object& BASE, void (Program::*CALLB)(), string TXT) :
	Object(BASE)
{
	callback = CALLB;
	text = TXT;
}
ButtonText::~ButtonText() {}
