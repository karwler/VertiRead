#include "engine/world.h"

// OBJECT

Object::Object(vec2i ANC, vec2i POS, vec2i SIZ, bool FX, bool FY, bool KW, bool KH, EColor CLR) :
	color(CLR),
	fixX(FX), fixY(FY),
	keepWidth(KW), keepHeight(KH)
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
	ret.x = fixX ? anchor.x : pixX(anchor.x);
	ret.y = fixY ? anchor.y : pixY(anchor.y);
	return ret;
}

void Object::Anchor(vec2i newPos) {
	anchor.x = fixX ? newPos.x : prcX(newPos.x);
	anchor.y = fixY ? newPos.y : prcY(newPos.y);
}

vec2i Object::Pos() const {
	vec2i ret = Anchor();
	ret.x += keepWidth ? pos.x : pixX(pos.x);
	ret.y += keepHeight ? pos.y : pixY(pos.y);
	return ret;
}

void Object::Pos(vec2i newPos) {
	vec2i dist = newPos - Anchor();
	pos.x = keepWidth ? dist.x : prcX(dist.x);
	pos.y = keepHeight ? dist.y : prcY(dist.y);
}

vec2i Object::End() const {
	vec2i ret = Anchor();
	ret.x += keepWidth ? end.x : pixX(end.x);
	ret.y += keepHeight ? end.y : pixY(end.y);
	return ret;
}

void Object::End(vec2i newPos) {
	vec2i dist = newPos - Anchor();
	end.x = keepWidth ? dist.x : prcX(dist.x);
	end.y = keepHeight ? dist.y : prcY(dist.y);
}

vec2i Object::Size() const {
	return End() - Pos();
}

void Object::Size(vec2i newSize) {
	vec2i dist = Pos() + newSize - Anchor();
	end.x = keepWidth ? dist.x : prcX(dist.x);
	end.y = keepHeight ? dist.y : prcY(dist.y);
}

// TEXTBOX

TextBox::TextBox(const Object& BASE, const Text& TXT, vec4i MRGN, int SPC) :
	Object(BASE),
	margin(MRGN),
	spacing(SPC)
{
	setText(TXT);
}
TextBox::~TextBox() {}

vector<Text> TextBox::getLines() const {
	vector<Text> lines;
	lines.push_back(Text("", vec2i(Pos().x+margin.z, Pos().y+margin.x), text.size, text.color));

	TTF_Font* font = TTF_OpenFont(World::winSys()->Settings().font.c_str(), text.size);
	int ypos = Pos().y + margin.x;
	uint lineCount = 0;
	string line;
	for (uint i = 0; i != text.text.size(); i++) {
		if (text.text[i] == '\n') {
			lineCount++;
			int curHeight;
			TTF_SizeText(font, line.c_str(), nullptr, &curHeight);
			ypos += curHeight + spacing;
			lines.push_back(Text("", vec2i(Pos().x + margin.z, ypos), text.size, text.color));
		}
		else
			lines[lineCount].text += text.text[i];
	}
	TTF_CloseFont(font);
	return lines;
}

void TextBox::setText(Text TXT) {
	text = TXT;
	Size(CalculateSize());
}

void TextBox::setSize(int val, bool byWidth) {
	vec2i scale = CalculateSize();
	float factor = (byWidth) ? val / scale.x : val / scale.y;
	text.size *= factor;
	Size(CalculateSize());
}

vec2i TextBox::CalculateSize() const {
	TTF_Font* font = TTF_OpenFont(World::winSys()->Settings().font.c_str(), text.size);
	vec2i scale;
	int maxWidth = 0;
	uint lineCount = 0;
	string line;
	for (uint i = 0; i <= text.text.size(); i++) {
		if (text.text[i] == '\n' || i == text.text.length()) {
			int curWidth, curHeight;
			TTF_SizeText(font, line.c_str(), &curWidth, &curHeight);
			if (curWidth > maxWidth) {
				maxWidth = curWidth;
				line.clear();
			}
			scale.y += curHeight + spacing;
			lineCount++;
		}
		else
			line += text.text[i];
	}
	scale.x = maxWidth + margin.z + margin.a;
	scale.y += margin.x + margin.y - spacing;
	TTF_CloseFont(font);
	return scale;
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
	Button(BASE, CALLB),
	texes(TEXS),
	curTex(0)
{}
ButtonImage::~ButtonImage() {}

void ButtonImage::OnClick() {
	Button::OnClick();
	curTex++;
	if (curTex == texes.size())
		curTex = 0;
	World::engine->SetRedrawNeeded();
}

string ButtonImage::CurTex() const {
	return texes.empty() ? "" : Filer::dirTexs() + texes[curTex];
}

// BUTTON TEXT

ButtonText::ButtonText(const Object& BASE, void (Program::*CALLB)(), string TXT, EColor TCLR) :
	Button(BASE, CALLB),
	text(TXT), textColor(TCLR)
{}
ButtonText::~ButtonText() {}
