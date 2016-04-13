#include "engine/world.h"

// SCROLL AREA

ScrollArea::ScrollArea(const Object& BASE, int SPC, int BARW) :
	Object(BASE),
	barW(BARW),
	spacing(SPC),
	diffSliderMouseY(0),
	listY(0),
	selectedItem(nullptr)
{}
ScrollArea::~ScrollArea() {}

void ScrollArea::SetValues() {
	listL = Size().y < ListH() ? ListH() - Size().y : 0;
	sliderH = Size().y < ListH() ? Size().y * Size().y / ListH() : Size().y;
	CheckListY();
}

void ScrollArea::DragSlider(int ypos) {
	DragList((ypos - diffSliderMouseY - Pos().y) * listL / Size().y);
}

void ScrollArea::DragList(int ypos) {
	listY = ypos;
	CheckListY();
	World::engine->SetRedrawNeeded();
}

void ScrollArea::ScrollList(int ymov)  {
	DragList(listY + ymov);
}

SDL_Rect ScrollArea::Bar() const {
	return {End().x-barW, Pos().y, barW, Size().y};
}

SDL_Rect ScrollArea::Slider() const {
	return {End().x-barW, SliderY(), barW, sliderH};
}

int ScrollArea::SelectedItem() const {
	return -1;
}

int ScrollArea::ListY() const {
	return listY;
}

int ScrollArea::ListH() const {
	return listH;
}

int ScrollArea::ListL() const {
	return listL;
}

int ScrollArea::SliderY() const {
	return ListH() <= Size().y ? Pos().y : Pos().y + listY * Size().y / ListH();
}

int ScrollArea::SliderH() const {
	return sliderH;
}

void ScrollArea::CheckListY() {
	if (listY < 0)
		listY = 0;
	else if (listY > listL)
		listY = listL;
}

// LIST BOX

ListBox::ListBox(const Object& BASE, const vector<ListItem*>& ITMS, int IH, int SPC, int BARW) :
	ScrollArea(BASE, SPC, BARW),
	itemH(IH)
{
	if (!ITMS.empty())
		Items(ITMS);
}

ListBox::~ListBox() {
	Clear(items);
}

void ListBox::SetValues() {
	listH = items.size() * (itemH + spacing) - spacing;
	ScrollArea::SetValues();
}

SDL_Rect ListBox::ItemRect(int i, SDL_Rect* Crop, EColor* color) const {
	SDL_Rect rect = {Pos().x, Pos().y - listY + i * (itemH + spacing), Size().x-barW, itemH};
	SDL_Rect crop = getCrop(rect, getRect());
	if (Crop)
		*Crop = crop;
	if (color)
		*color = items[i] == selectedItem ? EColor::highlighted : EColor::rectangle;
	return {rect.x, rect.y+crop.y, rect.w, rect.h-crop.y-crop.h};	// ignore horizontal crop
}

int ListBox::SelectedItem() const {
	for (int i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return -1;
}

vec2i ListBox::VisibleItems() const {
	vec2i interval(listY / (itemH + spacing));
	interval.y = listH > Size().y ? (Size().y+listY) / (itemH+spacing) : items.size()-1;
	return interval;
}

const vector<ListItem*>& ListBox::Items() const {
	return items;
}

void ListBox::Items(const vector<ListItem*>& objects) {
	Clear(items);
	items = objects;
	SetValues();
}

int ListBox::ItemH() const {
	return itemH;
}

// TILE BOX

TileBox::TileBox(const Object& BASE, const vector<ListItem*>& ITMS, vec2i TS, int BARW) :
	ScrollArea(BASE, TS.y/5, BARW),
	tileSize(TS)
{
	if (!ITMS.empty())
		Items(ITMS);
}

TileBox::~TileBox() {
	Clear(items);
}

void TileBox::SetValues() {
	dim.x = Size().x - barW > tileSize.x + spacing ? (Size().x - barW) / (tileSize.x + spacing) : 1;	// column count
	dim.y = items.size() > dim.x ? items.size() / dim.x : 1;											// row count
	listH = dim.y * (tileSize.y + spacing) - spacing;
	ScrollArea::SetValues();
}

SDL_Rect TileBox::ItemRect(int i, SDL_Rect* Crop, EColor* color) const {
	SDL_Rect rect = {(i - (i/dim.x) * dim.x) * (tileSize.x+spacing) + Pos().x, (i/dim.x) * (tileSize.y+spacing) + Pos().y - listY, tileSize.x, tileSize.y};
	SDL_Rect crop = getCrop(rect, getRect());
	if (Crop)
		*Crop = crop;
	if (color)
		*color = items[i] == selectedItem ? EColor::highlighted : EColor::rectangle;
	return {rect.x, rect.y+crop.y, rect.w-crop.w, rect.h-crop.y-crop.h};	// ignore left x side crop
}

int TileBox::SelectedItem() const {
	for (int i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return -1;
}

vec2i TileBox::VisibleItems() const {
	vec2i interval((listY+spacing) / (tileSize.y+spacing) * dim.x);
	interval.y = listH > Size().y ? (Size().y+listY) / (tileSize.y+spacing) * dim.x + dim.x-1 : items.size()-1;
	return interval;
}

const vector<ListItem*>& TileBox::Items() const {
	return items;
}

void TileBox::Items(const vector<ListItem*>& objects) {
	Clear(items);
	items = objects;
	SetValues();
}

vec2i TileBox::TileSize() const {
	return tileSize;
}

vec2i TileBox::Dim() const {
	return dim;
}

// READER BOX

ReaderBox::ReaderBox(const vector<Texture*> PICS, string CURPIC, float ZOOM) :
	ScrollArea(Object(0, 0, World::winSys()->Resolution(), FIX_POS | FIX_END, EColor::background), 10),
	sliderFocused(false),
	hideDelay(0.6f),
	sliderTimer(1.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0),
	blistW(48),
	playerW(400)
{
	if (!PICS.empty())
		Pictures(PICS, CURPIC);

	vec2i arcT(Pos());
	vec2i posT(-1, -1);
	vec2i sizT(blistW, blistW);
	listButtons = {
		ButtonImage(Object(arcT,                    posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_NextDir, {"next_dir"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x),   posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_PrevDir, {"prev_dir"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*2), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_ZoomIn, {"zoom_in"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*3), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_ZoomOut, {"zoom_out"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*4), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_ZoomReset, {"zoom_r"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*5), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_CenterView, {"center"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*6), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_Back, {"back"}),
	};

	arcT += vec2i(Size().x/2, Size().y);
	posT = vec2i(arcT.x-blistW/2, arcT.y-blistW);
	vec2i ssizT(32, 32);
	vec2i sposT(posT.x, arcT.y-blistW/2-ssizT.y/2);
	playerButtons = {
		ButtonImage(Object(arcT, posT,                    sizT, FIX_Y | FIX_SIZ), &Program::Event_PlayPause, {"play", "pause"}),
		ButtonImage(Object(arcT, posT-vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::Event_PrevSong, {"prev_song"}),
		ButtonImage(Object(arcT, posT+vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::Event_NextSong, {"next_song"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20,           0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_Mute, {"mute"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x,   0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_VolumeDown, {"vol_down"}),
		ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x*2, 0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_VolumeUp, {"vol_up"})
	};
}

ReaderBox::~ReaderBox() {
	World::library()->ClearPics();
}

void ReaderBox::SetValues() {
	listXL = (Size().x < ListW()) ? (ListW() - Size().x) /2: 0;
	CheckListX();
	ScrollArea::SetValues();
}

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
	CheckListX();
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

vector<ButtonImage>& ReaderBox::PlayerButtons() {
	return playerButtons;
}

Image ReaderBox::getImage(int i, SDL_Rect* Crop) const {
	Image img = pics[i];
	img.size = vec2i(pics[i].size.x*zoom, pics[i].size.y*zoom);
	img.pos = vec2i(Pos().x + Size().x/2 - img.size.x/2 - listX, pics[i].pos.y*zoom - listY);
	SDL_Rect crop = getCrop(img.getRect(), getRect());
	if (Crop)
		*Crop = crop;
	return img;
}

vec2i ReaderBox::VisiblePictures() const {
	vec2i interval(0, pics.size()-1);
	for (int i=interval.x; i<=interval.y; i++)
		if ((pics[i].pos.y + pics[i].size.y)*zoom >= listY) {
			interval.x = i;
			break;
		}
	for (int i=interval.x; i<=interval.y; i++) {
		if (pics[i].pos.y*zoom > listY + Size().y) {
			interval.y = i-1;
			break;
		}
	}
	return interval;
}

const vector<Image>& ReaderBox::Pictures() const {
	return pics;
}

void ReaderBox::Pictures(const vector<Texture*>& pictures, string curPic) {
	listH = 0;
	listW = 0;
	int startPos = 0;
	for (Texture* it : pictures) {
		pics.push_back(Image(vec2i(0, listH), it));
		if (it->File() == curPic)
			startPos = listH;
		if (it->Res().x > listW)
			listW = it->Res().x;
		listH += it->Res().y + spacing;
	}
	listY = startPos;
	SetValues();
}

int ReaderBox::ListX() const {
	return listX;
}

int ReaderBox::ListW() const {
	return listW * zoom;
}

int ReaderBox::ListH() const {
	return listH * zoom;
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

void ReaderBox::CheckListX() {
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
}
