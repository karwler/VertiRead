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
	int sizY = Size().y;
	int lstH = ListH();

	listL = sizY < lstH ? lstH - sizY : 0;
	sliderH = sizY < lstH ? sizY * sizY / lstH : sizY;
	CheckListY();
}

void ScrollArea::DragSlider(int ypos) {
	DragList((ypos - diffSliderMouseY - Pos().y) * listH / Size().y);
}

void ScrollArea::DragList(int ypos) {
	listY = ypos;
	CheckListY();
	World::engine()->SetRedrawNeeded();
}

void ScrollArea::ScrollList(int ymov)  {
	DragList(listY + ymov);
}

SDL_Rect ScrollArea::Bar() const {
	return {End().x-barW, Pos().y, BarW(), Size().y};
}

SDL_Rect ScrollArea::Slider() const {
	return { End().x-barW, SliderY(), BarW(), sliderH };
}

btsel ScrollArea::SelectedItem() const {
	return btsel();
}

int ScrollArea::BarW() const {
	return (sliderH == Size().y) ? 0 : barW;
}

int ScrollArea::ListY() const {
	return listY;
}

int ScrollArea::ListH() const {
	return listH;
}

float ScrollArea::Zoom() const {
	return 1.f;
}

int ScrollArea::ListL() const {
	return listL;
}

int ScrollArea::SliderY() const {
	int sizY = Size().y;
	int lstH = ListH();

	return lstH <= sizY ? Pos().y : Pos().y + listY * sizY / lstH;
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
	clear(items);
}

ListBox* ListBox::Clone() const {
	return new ListBox(*this);
}

ListItem* ListBox::Item(int id) const {
	return items[id];
}

SDL_Rect ListBox::ItemRect(int i, SDL_Rect* Crop, EColor* color) const {
	vec2i pos = Pos();
	SDL_Rect rect = { pos.x, pos.y - listY + i * (itemH + spacing), Size().x-BarW(), itemH };
	SDL_Rect crop = getCrop(rect, getRect());

	if (Crop)
		*Crop = crop;
	if (color)
		*color = items[i] == selectedItem ? EColor::highlighted : EColor::rectangle;
	return cropRect(rect, crop);
}

btsel ListBox::SelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t ListBox::VisibleItems() const {
	if (items.size() == 0)
		return vec2t(1, 0);
	
	int sizY = Size().y;
	return vec2t(listY / (itemH + spacing), (listH > sizY) ? (sizY+listY) / (itemH+spacing) : items.size()-1);
}

const vector<ListItem*>& ListBox::Items() const {
	return items;
}

void ListBox::Items(const vector<ListItem*>& objects) {
	clear(items);
	items = objects;

	listH = items.size() * (itemH + spacing) - spacing;
	SetValues();
}

int ListBox::ItemH() const {
	return itemH;
}

// TILE BOX

TileBox::TileBox(const Object& BASE, const vector<ListItem*>& ITMS, const vec2i& TS, int BARW) :
	ScrollArea(BASE, TS.y/5, BARW),
	tileSize(TS)
{
	if (!ITMS.empty())
		Items(ITMS);
}

TileBox::~TileBox() {
	clear(items);
}

TileBox* TileBox::Clone() const {
	return new TileBox(*this);
}

void TileBox::SetValues() {
	int sizX = Size().x;
	dim.x = (sizX - barW > tileSize.x + spacing) ? (sizX - barW) / (tileSize.x + spacing) : 1;	// column count
	dim.y = (items.size() > dim.x) ? items.size() / dim.x : 1;									// row count
	listH = dim.y * (tileSize.y + spacing) - spacing;
	ScrollArea::SetValues();
}

SDL_Rect TileBox::ItemRect(int id, SDL_Rect* Crop, EColor* color) const {
	vec2i pos = Pos();
	SDL_Rect rect = {(id - (id/dim.x) * dim.x) * (tileSize.x+spacing) + pos.x, (id/dim.x) * (tileSize.y+spacing) + pos.y - listY, tileSize.x, tileSize.y};
	SDL_Rect crop = getCrop(rect, getRect());

	if (Crop)
		*Crop = crop;
	if (color)
		*color = (items[id] == selectedItem) ? EColor::highlighted : EColor::rectangle;
	return cropRect(rect, crop);
}

btsel TileBox::SelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t TileBox::VisibleItems() const {
	if (items.size() == 0)
		return vec2t(1, 0);
	
	int sizY = Size().y;
	return vec2t((listY+spacing) / (tileSize.y+spacing) * dim.x, (listH > sizY) ? (sizY+listY) / (tileSize.y+spacing) * dim.x + dim.x-1 : items.size()-1);
}

const vector<ListItem*>& TileBox::Items() const {
	return items;
}

void TileBox::Items(const vector<ListItem*>& objects) {
	clear(items);
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

ReaderBox::ReaderBox(const Object& BASE, const vector<Texture*>& PICS, const string& CURPIC, float ZOOM) :
	ScrollArea(BASE, 10),
	sliderFocused(false),
	hideDelay(0.6f),
	mouseTimer(hideDelay), sliderTimer(0.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0),
	blistW(48),
	playerW(400)
{
	if (!PICS.empty())
		Pictures(PICS, CURPIC);

	vec2i arcT(Pos());
	vec2i posT(-1);
	vec2i sizT(blistW);
	listButtons = {
		ButtonImage(Object(arcT,                    posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_NextDir, {"next_dir"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x),   posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_PrevDir, {"prev_dir"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*2), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomIn, {"zoom_in"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*3), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomOut, {"zoom_out"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*4), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomReset, {"zoom_r"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*5), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_CenterView, {"center"}),
		ButtonImage(Object(arcT+vec2i(0, sizT.x*6), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, {"back"}),
	};

	arcT += vec2i(Size().x/2, Size().y);
	posT = vec2i(arcT.x-blistW/2, arcT.y-blistW);
	vec2i ssizT(32);
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
	if (!World::winSys()->ShowMouse())
		World::winSys()->ShowMouse(true);
}

ReaderBox* ReaderBox::Clone() const {
	return new ReaderBox(*this);
}

void ReaderBox::SetValues() {
	listXL = (Size().x < ListW()) ? (ListW() - Size().x) /2: 0;
	CheckListX();
	ScrollArea::SetValues();
}

void ReaderBox::Tick(float dSec) {
	// check and set UI visibility
	CheckMouseShow(dSec);
	CheckMouseOverSlider(dSec);
	CheckMouseOverList(dSec);
	CheckMouseOverPlayer(dSec);
}

void ReaderBox::CheckMouseShow(float dSec) {
	if (inRect(getRect(), InputSys::mousePos())) {
		if (World::inputSys()->mouseMove().isNull()) {
			if (mouseTimer != 0.f && !showSlider() && !showList() && !showPlayer()) {
				mouseTimer -= dSec;
				if (mouseTimer <= 0.f) {
					mouseTimer = 0.f;
					World::winSys()->ShowMouse(false);
					World::engine()->SetRedrawNeeded();
				}
			}
		}
		else if (mouseTimer != hideDelay) {
			mouseTimer = hideDelay;
			World::winSys()->ShowMouse(true);
			World::engine()->SetRedrawNeeded();
		}
	}
	else if (!World::winSys()->ShowMouse())
		World::winSys()->ShowMouse(true);
}

void ReaderBox::CheckMouseOverSlider(float dSec) {
	if (inRect(Bar(), InputSys::mousePos())) {
		if (sliderTimer != hideDelay) {
			sliderTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
	}
	else if (sliderTimer != 0.f && !sliderFocused) {
		sliderTimer -= dSec;
		if (sliderTimer <= 0.f) {
			sliderTimer = 0.f;
			World::engine()->SetRedrawNeeded();
		}
	}
}

void ReaderBox::CheckMouseOverList(float dSec) {
	vec2i mPos = InputSys::mousePos();
	SDL_Rect bRect = List();

	if ((showList() && inRect(bRect, mPos)) || (!showList() && inRect({bRect.x, bRect.y, bRect.w/4, bRect.h}, mPos))) {
		if (listTimer != hideDelay) {
			listTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
	}
	else if (listTimer != 0.f) {
		listTimer -= dSec;
		if (listTimer <= 0.f) {
			listTimer = 0.f;
			World::engine()->SetRedrawNeeded();
		}
	}
}

void ReaderBox::CheckMouseOverPlayer(float dSec) {
	vec2i mPos = InputSys::mousePos();
	SDL_Rect bRect = Player();
	int pInitH = bRect.h/4;

	if ((showPlayer() && inRect(bRect, mPos)) || (!showPlayer() && inRect({bRect.x, bRect.y+bRect.h-pInitH, bRect.w, pInitH}, mPos))) {
		if (playerTimer != hideDelay) {
			playerTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
	}
	else if (playerTimer != 0.f) {
		playerTimer -= dSec;
		if (playerTimer <= 0.f) {
			playerTimer = 0.f;
			World::engine()->SetRedrawNeeded();
		}
	}
}

void ReaderBox::DragListX(int xpos) {
	listX = xpos;
	CheckListX();
	World::engine()->SetRedrawNeeded();
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

	World::engine()->SetRedrawNeeded();
}

void ReaderBox::MultZoom(float zfactor) {
	Zoom(zoom * zfactor);
}

void ReaderBox::DivZoom(float zfactor) {
	Zoom(zoom / zfactor);
}

SDL_Rect ReaderBox::List() const {
	vec2i pos = Pos();
	return {pos.x, pos.y, blistW, int(listButtons.size())*blistW};
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

Image ReaderBox::getImage(size_t i, SDL_Rect* Crop) const {
	Image img = pics[i];
	img.size = vec2i(pics[i].size.x*zoom, pics[i].size.y*zoom);
	img.pos = vec2i(Pos().x + Size().x/2 - img.size.x/2 - listX, pics[i].pos.y*zoom - listY);
	
	if (Crop)
		*Crop = getCrop(img.getRect(), getRect());
	return img;
}

vec2t ReaderBox::VisiblePictures() const {
	int sizY = Size().y;
	vec2t interval(0, pics.size()-1);

	for (size_t i=interval.x; i<=interval.y; i++)
		if ((pics[i].pos.y + pics[i].size.y)*zoom >= listY) {
			interval.x = i;
			break;
		}
	for (size_t i=interval.x; i<=interval.y; i++)
		if (pics[i].pos.y*zoom > listY + sizY) {
			interval.y = i-1;
			break;
		}
	return interval;
}

const vector<Image>& ReaderBox::Pictures() const {
	return pics;
}

void ReaderBox::Pictures(const vector<Texture*>& pictures, const string& curPic) {
	listH = 0;
	listW = 0;
	listY = 0;
	for (Texture* it : pictures) {
		pics.push_back(Image(vec2i(0, listH), it));
		if (it->File() == curPic)
			listY = listH;
		if (it->Res().x > listW)
			listW = it->Res().x;
		listH += it->Res().y + spacing;
	}
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

float ReaderBox::Zoom() const {
	return zoom;
}

bool ReaderBox::showMouse() const {
	return mouseTimer != 0.f;
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
