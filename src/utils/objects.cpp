#include "engine/world.h"

// OBJECT

Object::Object(vec2f POS, vec2f SIZ, EColor CLR) :
	pos(POS), size(SIZ),
	color(CLR)
{}

Object::Object(vec2i POS, vec2i SIZ, EColor CLR) :
	pos(prc(POS)), size(prc(SIZ)),
	color(CLR)
{}

Object::~Object() {}

SDL_Rect Object::getRect() const {
	return {Pos().x, Pos().y, Size().x, Size().y};
}

vec2i Object::Pos() const {
	return pix(pos);
}

vec2i Object::Size() const {
	return pix(size);
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
	size = prc(CalculateSize());
}

void TextBox::setSize(int val, bool byWidth) {
	vec2i scale = CalculateSize();
	float factor = (byWidth) ? val / scale.x : val / scale.y;
	text.size *= factor;
	size = prc(CalculateSize());
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

void Button::setCallback(void (Program::*func)()) {
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

// SCROLL AREA

ScrollArea::ScrollArea(const Object& BASE, int SPC, int BARW) :
	Object(BASE),
	barW(BARW),
	diffSliderMouseY(0),
	spacing(SPC),
	listY(0)
{}
ScrollArea::~ScrollArea() {}

void ScrollArea::DragSlider(int ypos) {
	DragList(prcY(ypos - diffSliderMouseY) * float(listH) / size.y);
}

void ScrollArea::DragList(int ypos) {
	listY = ypos;
	if (listY < 0)
		listY = 0;
	else if (listY > listL)
		listY = listL;
	World::winSys()->DrawScene();
}

void ScrollArea::ScrollList(int ymov)  {
	DragList(listY + ymov);
}

void ScrollArea::SetValues() {
	SetScrollValues();
	if (listY < 0)
		listY = 0;
	else if (listY > listL)
		listY = listL;
}

SDL_Rect ScrollArea::Bar() const {
	return{ pixX(pos.x + size.x) - barW, pixY(pos.y), barW, pixY(size.y) };
}

SDL_Rect ScrollArea::Slider() const {
	return{ pixX(pos.x + size.x) - barW, SliderY(), barW, pixY(sliderH) };
}

int ScrollArea::Spacing() const {
	return spacing;
}

int ScrollArea::ListY() const {
	return listY;
}

float ScrollArea::sliderY() const {
	return float(listY) * size.y / float(listH);
}

int ScrollArea::SliderY() const {
	return pixY(pos.y + sliderY());
}

int ScrollArea::SliderH() const {
	return pixY(sliderH);
}

void ScrollArea::SetScrollValues() {
	listL = (pixY(size.y) < listH) ? listH - pixY(size.y) : 0;
	sliderH = (listH > pixY(size.y)) ? size.y * size.y / prcY(listH) : size.y;
}

// LIST BOX

ListBox::ListBox(const Object& BASE, const vector<ListItem*>& ITMS, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW)
{
	if (!ITMS.empty())
		setItems(ITMS);
}

ListBox::~ListBox() {
	Clear(items);
}

void ListBox::SetValues() {
	listH = items.size() * spacing - spacing;
	for (ListItem* it : items)
		listH += it->height;
	ScrollArea::SetValues();
}

vector<ListItem*> ListBox::Items() const {
	return items;
}

void ListBox::setItems(const vector<ListItem*>& objects) {
	Clear(items);
	items = objects;
	SetValues();
}

// TILE BOX

TileBox::TileBox(const Object& BASE, const vector<TileItem>& ITMS, vec2i TS, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW),
	tileSize(TS)
{
	spacing = TS.y / 5;
	if (!ITMS.empty())
		setItems(ITMS);
}
TileBox::~TileBox() {}

void TileBox::SetValues() {
	tilesPerRow = (pixX(size.x) - barW) / (tileSize.x + spacing);
	if (tilesPerRow == 0)
		tilesPerRow = 1;
	float height = float((items.size() * (tileSize.y + spacing)) - spacing) / tilesPerRow;
	listH = (height / 2.f == int(height / 2.f)) ? height : height + tileSize.y;
	ScrollArea::SetValues();
}

vector<TileItem>& TileBox::Items() {
	return items;
}

void TileBox::setItems(const vector<TileItem>& objects) {
	items = objects;
	SetValues();
}

vec2i TileBox::TileSize() const {
	return tileSize;
}

int TileBox::TilesPerRow() const {
	return tilesPerRow;
}

// READER BOX

ReaderBox::ReaderBox(const vector<string>& PICS, float ZOOM) :
	ScrollArea(Object(vec2i(0, 0), World::winSys()->Resolution(), EColor::background), 10),
	sliderFocused(false),
	hideDelay(2.f),
	sliderTimer(1.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0)
{
	if (!PICS.empty())
		SetPictures(PICS);
}
ReaderBox::~ReaderBox() {}

void ReaderBox::Tick(float dSec) {
	vec2i mPos = InputSys::mousePos();

	if (inRect(Bar(), mPos) && sliderTimer != hideDelay) {
		sliderTimer = hideDelay;
		World::winSys()->DrawScene();
	}
	else if (showSlider() && !sliderFocused)
		if ((sliderTimer -= dSec) <= 0.f) {
			sliderTimer = 0.f;
			World::winSys()->DrawScene();
		}

	if (inRect(List(), mPos) && listTimer != hideDelay) {
		listTimer = hideDelay;
		World::winSys()->DrawScene();
	}
	else if (showList())
		if ((listTimer -= dSec) < 0.f) {
			listTimer = 0.f;
			World::winSys()->DrawScene();
		}

	if (inRect(Player(), mPos) && playerTimer != hideDelay) {
		playerTimer = hideDelay;
		World::winSys()->DrawScene();
	}
	else if (showPlayer())
		if ((playerTimer -= dSec) < 0.f) {
			playerTimer = 0.f;
			World::winSys()->DrawScene();
		}
}

void ReaderBox::DragListX(int xpos) {
	listX = xpos;
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
	World::winSys()->DrawScene();
}

void ReaderBox::ScrollListX(int xmov) {
	DragListX(listX + xmov);
}

void ReaderBox::Zoom(float factor) {
	// correct xy position
	listY = float(listY) * factor / zoom;
	listX = float(listX) * factor / zoom;
	zoom = factor;
	SetValues();
	World::winSys()->DrawScene();
}

void ReaderBox::AddZoom(float zadd) {
	Zoom(zoom + zadd);
}

SDL_Rect ReaderBox::List() const {
	return { pixX(pos.x), pixY(pos.y), 100, pixY(size.y)-100 };	// need to get rid of the hard coding
}

SDL_Rect ReaderBox::Player() const {
	return { pixX(pos.x)+100, pixY(pos.y)+pixY(size.y)-100, pixX(size.x)-200, 100 };
}

void ReaderBox::SetValues() {
	int maxWidth = 0;
	int ypos = 0;
	for (Image& it : pics) {
		// set sizes and position
		vec2i res = World::winSys()->GetTextureSize(it.texname);
		it.size = vec2i(float(res.x) * zoom, float(res.y) * zoom);
		it.pos.y = ypos;

		if (it.size.x > maxWidth)
			maxWidth = it.size.x;
		ypos += it.size.y + spacing;
	}
	// calculate slider and limits related variables
	listH = ypos;
	listXL = (pixX(size.x) < maxWidth) ? (maxWidth - pixX(size.x)) /2: 0;
	ScrollArea::SetValues();
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
}

const vector<Image>& ReaderBox::Pictures() const {
	return pics;
}

void ReaderBox::SetPictures(const vector<string>& pictures) {
	for (const string& it : pictures) {
		vec2i res = World::winSys()->GetTextureSize(it);
		if (res.x != 0 && res.y != 0)	// don't add broken images
			pics.push_back(Image(vec2i(), vec2i(), it));
	}
	listY = 0;
	SetValues();
}

int ReaderBox::ListX() const {
	return listX;
}

bool ReaderBox::showSlider() const {
	return sliderTimer != 0.f;
}

bool ReaderBox::showList() const {
	return listTimer != 0.f;
}

bool ReaderBox::showPlayer() const {
	return playerTimer != 0.f;
}
