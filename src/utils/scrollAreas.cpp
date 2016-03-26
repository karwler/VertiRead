#include "engine/world.h"

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
	DragList((ypos - diffSliderMouseY) * listH / Size().y);
}

void ScrollArea::DragList(int ypos) {
	listY = ypos;
	if (listY < 0)
		listY = 0;
	else if (listY > listL)
		listY = listL;
	World::engine->SetRedrawNeeded();
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
	return {End().x-barW, Pos().y, barW, Size().y};
}

SDL_Rect ScrollArea::Slider() const {
	return {End().x-barW, SliderY(), barW, sliderH};
}

int ScrollArea::Spacing() const {
	return spacing;
}

int ScrollArea::ListY() const {
	return listY;
}

int ScrollArea::SliderY() const {
	return (listH == 0) ? 0 : Pos().y + listY * Size().y / listH;
}

int ScrollArea::SliderH() const {
	return sliderH;
}

void ScrollArea::SetScrollValues() {
	listL = (Size().y < listH) ? listH - Size().y : 0;
	sliderH = (listH > Size().y) ? Size().y * Size().y / listH : Size().y;
}

// LIST BOX

ListBox::ListBox(const Object& BASE, const vector<ListItem*>& ITMS, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW)
{
	if (!ITMS.empty())
		Items(ITMS);
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

const vector<ListItem*>& ListBox::Items() const {
	return items;
}

void ListBox::Items(const vector<ListItem*>& objects) {
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
		Items(ITMS);
}
TileBox::~TileBox() {}

void TileBox::SetValues() {
	tilesPerRow = (Size().x - barW) / (tileSize.x + spacing);
	if (tilesPerRow == 0)
		tilesPerRow = 1;
	float height = float((items.size() * (tileSize.y + spacing)) - spacing) / tilesPerRow;
	listH = (height / 2.f == int(height / 2.f)) ? height : height + tileSize.y;
	ScrollArea::SetValues();
}

vector<TileItem>& TileBox::Items() {
	return items;
}

void TileBox::Items(const vector<TileItem>& objects) {
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
	ScrollArea(Object(vec2i(), vec2i(), World::winSys()->Resolution(), true, true, false, false, EColor::background), 10),
	sliderFocused(false),
	hideDelay(0.6f),
	sliderTimer(1.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0),
	blistW(48),
	playerW(400)
{
	if (!PICS.empty())
		Pictures(PICS);

	vec2i arcT(Pos());
	vec2i posT(-1, -1);
	vec2i sizT(blistW, blistW);
	listButtons = {
		ButtonImage(Object(arcT,                    posT, sizT, true, true, true, true), &Program::Event_NextDir, {"next_dir.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x),   posT, sizT, true, true, true, true), &Program::Event_PrevDir, {"prev_dir.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*2), posT, sizT, true, true, true, true), &Program::Event_ZoomIn, {"zoom_in.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*3), posT, sizT, true, true, true, true), &Program::Event_ZoomOut, {"zoom_out.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*4), posT, sizT, true, true, true, true), &Program::Event_ZoomReset, {"zoom_r.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*5), posT, sizT, true, true, true, true), &Program::Event_CenterView, {"center.png"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*6), posT, sizT, true, true, true, true), &Program::Event_Back, {"back.png"}),
	};

	arcT += vec2i(Size().x/2, Size().y);
	posT = vec2i(arcT.x-blistW/2, arcT.y-blistW);
	vec2i ssizT(32, 32);
	vec2i sposT(posT.x, arcT.y-blistW/2-ssizT.y/2);
	playerButtons = {
		ButtonImage(Object(arcT, posT,                    sizT, false, true, true, true), &Program::Event_PlayPause, {"play.png", "pause.png"}),
		ButtonImage(Object(arcT, posT-vec2i(sizT.x, 0),   sizT, false, true, true, true), &Program::Event_PrevSong, {"prev_song.png"}),
		ButtonImage(Object(arcT, posT+vec2i(sizT.x, 0),   sizT, false, true, true, true), &Program::Event_NextSong, {"next_song.png"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20,           0), ssizT, false, true, true, true), &Program::Event_Mute, {"mute.png"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x,   0), ssizT, false, true, true, true), &Program::Event_VolumeDown, {"vol_down.png"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x*2, 0), ssizT, false, true, true, true), &Program::Event_VolumeUp, {"vol_up.png"})
	};
}
ReaderBox::~ReaderBox() {}

void ReaderBox::Tick() {
	if (!CheckMouseOverSlider())
		if (!CheckMouseOverList())
			CheckMouseOverPlayer();
}

bool ReaderBox::CheckMouseOverSlider() {
	if (inRect(Bar(), InputSys::mousePos()) && sliderTimer != hideDelay) {
		sliderTimer = hideDelay;
		World::engine->SetRedrawNeeded();
		return true;
	}
	if (showSlider() && !sliderFocused)
		if ((sliderTimer -= World::engine->deltaSeconds()) <= 0.f) {
			sliderTimer = 0.f;
			World::engine->SetRedrawNeeded();
			return true;
		}
	return false;
}

bool ReaderBox::CheckMouseOverList() {
	vec2i mPos = InputSys::mousePos();
	SDL_Rect bRect = List();

	if (((showList() && inRect(bRect, mPos)) || (!showList() && inRect({bRect.x, bRect.y, bRect.w/4, bRect.h}, mPos))) && listTimer != hideDelay) {
		listTimer = hideDelay;
		World::engine->SetRedrawNeeded();
		return true;
	}
	if (showList())
		if ((listTimer -= World::engine->deltaSeconds()) < 0.f) {
			listTimer = 0.f;
			World::engine->SetRedrawNeeded();
			return true;
		}
	return false;
}

bool ReaderBox::CheckMouseOverPlayer() {
	vec2i mPos = InputSys::mousePos();
	SDL_Rect bRect = Player();

	if (((showPlayer() && inRect(bRect, mPos)) || (!showPlayer() && inRect({bRect.x, bRect.y+bRect.h/10*9, bRect.w, bRect.h/6}, mPos))) && playerTimer != hideDelay) {
		playerTimer = hideDelay;
		World::engine->SetRedrawNeeded();
		return true;
	}
	if (showPlayer())
		if ((playerTimer -= World::engine->deltaSeconds()) < 0.f) {
			playerTimer = 0.f;
			World::engine->SetRedrawNeeded();
			return true;
		}
	return false;
}

void ReaderBox::DragListX(int xpos) {
	listX = xpos;
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
	World::engine->SetRedrawNeeded();
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
	World::engine->SetRedrawNeeded();
}

void ReaderBox::AddZoom(float zadd) {
	Zoom(zoom + zadd);
}

SDL_Rect ReaderBox::List() const {
	return {Pos().x, Pos().y, blistW, int(listButtons.size())*blistW};
}

vector<ButtonImage>&ReaderBox::ListButtons() {
	return listButtons;
}

SDL_Rect ReaderBox::Player() const {
	return {Pos().x+Size().x/2-playerW/2, End().y-62, playerW, 62};
}

vector<ButtonImage>&ReaderBox::PlayerButtons() {
	return playerButtons;
}

void ReaderBox::SetValues() {
	int maxWidth = 0;
	int ypos = 0;
	for (Image& it : pics) {
		// set sizes and position
		vec2i res = textureSize(it.texname);
		it.size = vec2i(float(res.x) * zoom, float(res.y) * zoom);
		it.pos.y = ypos;

		if (it.size.x > maxWidth)
			maxWidth = it.size.x;
		ypos += it.size.y + spacing;
	}
	// calculate slider and limits related variables
	listH = ypos;
	listXL = (Size().x < maxWidth) ? (maxWidth - Size().x) /2: 0;
	ScrollArea::SetValues();
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
}

const vector<Image>& ReaderBox::Pictures() const {
	return pics;
}

void ReaderBox::Pictures(const vector<string>& pictures) {
	for (const string& it : pictures) {
		vec2i res = textureSize(it);
		if (res.x != 0 && res.y != 0)
			pics.push_back(Image(vec2i(), res, it));
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
