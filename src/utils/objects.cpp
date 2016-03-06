#include "objects.h"
#include "engine/world.h"

// OBJECT

Object::Object(vec2i POS, vec2i SCL, EColor CLR, EObjectState STAT) :
	state(STAT),
	pos(POS), size(SCL),
	color(CLR)
{}
Object::~Object() {}

SDL_Rect Object::getRect() const {
	return {pos.x, pos.y, size.x, size.y};
}

vec2i Object::Size() const {
	return size;
}

void Object::setSize(vec2i newSize) {
	size = newSize;
}

// IMAGE

Image::Image(Object BASE, string TEXN) :
	Object(BASE),
	texname(TEXN)
{}
Image::~Image() {}

// TEXTBOX

TextBox::TextBox(Object BASE, Text TXT, vec4i MRGN, int SPC) :
	Object(BASE),
	margin(MRGN),
	spacing(SPC)
{
	setText(TXT);
}
TextBox::~TextBox() {}

vector<Text> TextBox::getLines() const {
	vector<Text> lines;
	lines.push_back(Text("", vec2i(pos.x+margin.z, pos.y+margin.x), text.size, text.color));

	TTF_Font* font = TTF_OpenFont(World::winSys()->Settings().font.c_str(), text.size);
	int ypos = pos.y + margin.x;
	uint lineCount = 0;
	string line;
	for (uint i = 0; i != text.text.size(); i++) {
		if (text.text[i] == '\n') {
			lineCount++;
			int curHeight;
			TTF_SizeText(font, line.c_str(), nullptr, &curHeight);
			ypos += curHeight + spacing;
			lines.push_back(Text("", vec2i(pos.x + margin.z, ypos), text.size, text.color));
		}
		else
			lines[lineCount].text += text.text[i];
	}
	TTF_CloseFont(font);
	return lines;
}

void TextBox::setText(Text TXT) {
	text = TXT;
	size = CalculateSize(text);
}

void TextBox::setSize(int val, bool byWidth) {
	vec2i scale = CalculateSize(text);
	float factor = (byWidth) ? val / scale.x : val / scale.y;
	text.size *= factor;
	size = CalculateSize(text);
}

vec2i TextBox::CalculateSize(Text TXT) const {
	TTF_Font* font = TTF_OpenFont(Filer::getFontPath(World::winSys()->Settings().font).string().c_str(), text.size);
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

Button::Button(Object BASE, void (Program::*CALLB)()) :
	Object(BASE),
	callback(CALLB)
{}
Button::~Button() {}

void Button::OnClick() {
	if (callback)
		(World::program()->*callback)();
}

void Button::setCallback(void (Program::*func)()) {
	callback = func;
}

// BUTTON IMAGE

ButtonImage::ButtonImage(Object BASE, void (Program::*CALLB)(), string TEXN) :
	Button(BASE, CALLB),
	texname(TEXN)
{}
ButtonImage::~ButtonImage() {}

// BUTTON TEXT

ButtonText::ButtonText(Object BASE, void (Program::*CALLB)(), string TXT, EColor TCLR) :
	Button(BASE, CALLB),
	text(TXT), textColor(TCLR)
{}
ButtonText::~ButtonText() {}

// SCROLL AREA

ScrollArea::ScrollArea(Object BASE, int SPC, int BARW) :
	Object(BASE),
	barW(BARW),
	spacing(SPC)
{}
ScrollArea::~ScrollArea() {}

void ScrollArea::ScrollSlider(float mov) {
	sliderY += mov;
	if (sliderY < 0.f)
		sliderY = 0.f;
	else if (sliderY > sliderL)
		sliderY = sliderL;
}

void ScrollArea::ScrollList(int mov)  {
	ScrollSlider(mov * size.y / listH);
}

SDL_Rect ScrollArea::Bar() const {
	return{ pos.x + size.x - barW, pos.y, barW, size.y };
}

SDL_Rect ScrollArea::Slider() const {
	return{ pos.x + size.x - barW, pos.y + int(sliderY), barW, sliderH };
}

int ScrollArea::listY() const {
	return (sliderL == 0) ? 0 : -sliderY * listL / sliderL;
}

int ScrollArea::Spacing() const {
	return spacing;
}

void ScrollArea::SetScollValues() {
	listL = listH - size.y;
	sliderH = (listH >= size.y) ? size.y * size.y / listH : size.y;
	sliderL = size.y - sliderH;
}

// LIST BOX

ListBox::ListBox(Object BASE, const vector<ListItem*>& ITMS, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW)
{
	if (!ITMS.empty())
		setItems(ITMS);
}

ListBox::~ListBox() {
	Clear(items);
}

vector<ListItem*> ListBox::Items() const {
	return items;
}

void ListBox::setItems(const vector<ListItem*>& objects) {
	Clear(items);
	items = objects;

	listH = items.size() * spacing - spacing;
	for (ListItem* it : objects)
		listH += it->height;
	SetScollValues();
}

// TILE BOX

TileBox::TileBox(Object BASE, const vector<TileItem>& ITMS, vec2i TS, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW),
	tileSize(TS)
{
	spacing = TS.y / 5;
	if (!ITMS.empty())
		setItems(ITMS);
}
TileBox::~TileBox() {}

vector<TileItem>& TileBox::Items() {
	return items;
}

void TileBox::setItems(const vector<TileItem>& objects) {
	items = objects;

	tilesPerRow = (size.x - barW) / (tileSize.x + spacing);
	if (tilesPerRow == 0)
		tilesPerRow = 1;
	float height = float((items.size() * (tileSize.y + spacing)) - spacing) / tilesPerRow;
	listH = (height / 2.f == int(height / 2.f)) ? height : height + tileSize.y;
	SetScollValues();
}

vec2i TileBox::TileSize() const {
	return tileSize;
}

int TileBox::TilesPerRow() const {
	return tilesPerRow;
}

// READER BOX

ReaderBox::ReaderBox(const vector<Image*>& PICS) :
	Object(vec2i(0, 0), World::winSys()->Resolution(), EColor::background)
{
	if (!PICS.empty())
		SetPictures(PICS);
}

ReaderBox::~ReaderBox() {
	Clear(pics);
}

vector<Image*> ReaderBox::Pictures() const {
	return pics;
}

void ReaderBox::SetPictures(const vector<Image*>& pictures) {

}
