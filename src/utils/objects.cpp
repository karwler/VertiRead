#include "engine/world.h"

EFixation operator|(EFixation a, EFixation b) {
	return static_cast<EFixation>(static_cast<byte>(a) | static_cast<byte>(b));
}

// OBJECT

Object::Object(vec2i POS, vec2i SIZ, EFixation FIX, EColor CLR) :
	fix(FIX),
	color(CLR)
{
	Pos(POS);
	Size(SIZ);
}
Object::~Object() {}

SDL_Rect Object::getRect() const {
	return {Pos().x, Pos().y, Size().x, Size().y};
}

vec2i Object::Pos() const {
	vec2i ret;
	ret.x = (fix & FIX_SX) ? pos.x : pixX(pos.x);
	ret.y = (fix & FIX_SY) ? pos.y : pixY(pos.y);
	return ret;
}

void Object::Pos(vec2i newPos) {
	pos.x = (fix & FIX_SX) ? newPos.x : prcX(newPos.x);
	pos.y = (fix & FIX_SY) ? newPos.y : prcY(newPos.y);
}

vec2i Object::End() const {
	vec2i ret;
	ret.x = (fix & FIX_EX) ? end.x : pixX(end.x);
	ret.y = (fix & FIX_EY) ? end.y : pixY(end.y);
	return ret;
}

void Object::End(vec2i newEnd) {
	end.x = (fix & FIX_EX) ? newEnd.x : prcX(newEnd.x);
	end.y = (fix & FIX_EY) ? newEnd.y : prcY(newEnd.y);
}

vec2i Object::Size() const {
	return vec2i(End().x - Pos().x, End().y - Pos().y);
}

void Object::Size(vec2i newSize) {
	End(Pos()+newSize);
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

ButtonImage::ButtonImage(const Object& BASE, void (Program::*CALLB)(), string TEXN) :
	Button(BASE, CALLB),
	texname(TEXN)
{}
ButtonImage::~ButtonImage() {}

// BUTTON TEXT

ButtonText::ButtonText(const Object& BASE, void (Program::*CALLB)(), string TXT, EColor TCLR) :
	Button(BASE, CALLB),
	text(TXT), textColor(TCLR)
{}
ButtonText::~ButtonText() {}
