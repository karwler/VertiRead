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

SDL_Rect ScrollArea::Bar() const {
	return {End().x-barW, Pos().y, BarW(), Size().y};
}

SDL_Rect ScrollArea::Slider() const {
	return { End().x-barW, SliderY(), BarW(), sliderH };
}

btsel ScrollArea::SelectedItem() const {
	return btsel();
}

vec2t ScrollArea::VisibleItems() const {
	return vec2t(1, 0);
}

// SCROLL AREA X1

ScrollAreaX1::ScrollAreaX1(const Object& BASE, int IH, int BARW) :
	ScrollArea(BASE, IH/6, BARW),
	itemH(IH)
{}
ScrollAreaX1::~ScrollAreaX1() {}

void ScrollAreaX1::SetValuesX1(size_t numRows) {
	listH = numRows * (itemH + spacing) - spacing;
	ScrollArea::SetValues();
}

int ScrollAreaX1::ItemH() const {
	return itemH;
}

// LIST BOX

ListBox::ListBox(const Object& BASE, const vector<ListItem*>& ITMS, int IH, int BARW) :
	ScrollAreaX1(BASE, IH, BARW)
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

void ListBox::SetValues() {
	SetValuesX1(items.size());
}

const vector<ListItem*>& ListBox::Items() const {
	return items;
}

void ListBox::Items(const vector<ListItem*>& objects) {
	clear(items);
	items = objects;
	SetValues();
}

ListItem* ListBox::Item(size_t id) const {
	return items[id];
}

SDL_Rect ListBox::ItemRect(size_t id) const {
	vec2i pos = Pos();
	SDL_Rect rect = { pos.x, pos.y - listY + id * (itemH+spacing), Size().x-BarW(), itemH };
	return cropRect(rect, getCrop(rect, getRect()));
}

SDL_Rect ListBox::ItemRect(size_t id, SDL_Rect& crop, EColor& color) const {
	vec2i pos = Pos();
	SDL_Rect rect = { pos.x, pos.y - listY + id * (itemH+spacing), Size().x-BarW(), itemH };

	color = items[id] == selectedItem ? EColor::highlighted : EColor::rectangle;
	crop = getCrop(rect, getRect());
	return cropRect(rect, crop);
}

btsel ListBox::SelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t ListBox::VisibleItems() const {
	int sizY = Size().y;
	if (items.empty() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t(listY / (itemH+spacing), (listH > sizY) ? (listY+sizY) / (itemH+spacing) : items.size()-1);
}

// TABLE BOX

TableBox::TableBox(const Object& BASE, const grid2<ListItem*>& ITMS, const vector<float>& IWS, int IH, int BARW) :
	ScrollAreaX1(BASE, IH, BARW),
	itemW(IWS)
{
	if (ITMS.arr())
		Items(ITMS);
}

TableBox::~TableBox() {
	clear(items);
}

TableBox* TableBox::Clone() const {
	return new TableBox(*this);
}

void TableBox::SetValues() {
	SetValuesX1(items.size().y);
}

const grid2<ListItem*>& TableBox::Items() const {
	return items;
}

void TableBox::Items(const grid2<ListItem*>& objects) {
	clear(items);
	items = objects;
	SetValues();
}

ListItem* TableBox::Item(size_t id) const {
	return items[id];
}

SDL_Rect TableBox::ItemRect(size_t id) const {
	SDL_Rect crop;
	return ItemRect(id, crop);
}

SDL_Rect TableBox::ItemRect(size_t id, SDL_Rect& crop) const {
	vec2i pos = Pos();
	vec2i siz = Size();
	vec2u loc = items.loc(id);
	float pref = 0.f;
	for (uint i=0; i!=loc.x; i++)
		pref += itemW[i];

	SDL_Rect rect = { pos.x + pref*float(siz.x) + spacing/2, pos.y - listY + loc.y * (itemH+spacing), itemW[loc.x]*float(siz.x) - spacing, itemH };
	crop = getCrop(rect, getRect());
	return cropRect(rect, crop);
}

btsel TableBox::SelectedItem() const {
	for (uint i=0; i!=items.length(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t TableBox::VisibleItems() const {
	int sizY = Size().y;
	if (!items.arr() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t((listY / (itemH+spacing)) * items.size().x, (listH > sizY) ? ((listY+sizY) / (itemH+spacing)) * items.size().x + items.size().x-1 : items.length()-1);
}

// TILE BOX

TileBox::TileBox(const Object& BASE, const vector<ListItem*>& ITMS, const vec2i& TS, int BARW) :
	ScrollArea(BASE, TS.y/6, BARW),
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

vec2i TileBox::TileSize() const {
	return tileSize;
}

vec2i TileBox::Dim() const {
	return dim;
}

const vector<ListItem*>& TileBox::Items() const {
	return items;
}

void TileBox::Items(const vector<ListItem*>& objects) {
	clear(items);
	items = objects;
	SetValues();
}

SDL_Rect TileBox::ItemRect(size_t id) const {
	vec2i pos = Pos();
	SDL_Rect rect = {(id - (id/dim.x) * dim.x) * (tileSize.x+spacing) + pos.x, (id/dim.x) * (tileSize.y+spacing) + pos.y - listY, tileSize.x, tileSize.y};
	return cropRect(rect, getCrop(rect, getRect()));
}

SDL_Rect TileBox::ItemRect(size_t id, SDL_Rect& crop, EColor& color) const {
	vec2i pos = Pos();
	SDL_Rect rect = {(id - (id/dim.x) * dim.x) * (tileSize.x+spacing) + pos.x, (id/dim.x) * (tileSize.y+spacing) + pos.y - listY, tileSize.x, tileSize.y};

	color = (items[id] == selectedItem) ? EColor::highlighted : EColor::rectangle;
	crop = getCrop(rect, getRect());
	return cropRect(rect, crop);
}

btsel TileBox::SelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t TileBox::VisibleItems() const {
	int sizY = Size().y;
	if (items.empty() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t(listY / (tileSize.y+spacing) * dim.x, (listH > sizY) ? (listY+sizY) / (tileSize.y+spacing) * dim.x + dim.x-1 : items.size()-1);
}

// READER BOX

ReaderBox::ReaderBox(const Object& BASE, const vector<Texture*>& PICS, const string& CURPIC, float ZOOM) :
	ScrollArea(BASE, 10),
	sliderFocused(false),
	mouseHideable(true),
	hideDelay(1.f),
	mouseTimer(hideDelay), sliderTimer(0.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0),
	blistW(48),
	playerW(400),
	playerH(72)
{
	if (!PICS.empty())
		Pictures(PICS, CURPIC);

	vec2i arcT(Pos());
	vec2i posT(-1);
	vec2i sizT(blistW);
	listObjects = {
		new ButtonImage(Object(arcT,                    posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_NextDir, {"next_dir"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x),   posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_PrevDir, {"prev_dir"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*2), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomIn, {"zoom_in"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*3), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomOut, {"zoom_out"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*4), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_ZoomReset, {"zoom_r"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*5), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_CenterView, {"center"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*6), posT, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, {"back"})
	};

	SDL_Rect player = Player();
	arcT += vec2i(Size().x/2, Size().y);
	posT = vec2i(arcT.x-sizT.x/2, arcT.y-sizT.y);
	vec2i ssizT(32);
	vec2i sposT(posT.x, arcT.y-blistW/2-ssizT.y/2);
	playerObjects = {
		new Label(Object(arcT, vec2i(player.x, player.y), vec2i(player.w, 20), FIX_Y | FIX_SIZ, EColor::darkened), "", ETextAlign::center),
		new ButtonImage(Object(arcT, posT,                    sizT, FIX_Y | FIX_SIZ), &Program::Event_PlayPause, {"play", "pause"}),
		new ButtonImage(Object(arcT, posT-vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::Event_PrevSong, {"prev_song"}),
		new ButtonImage(Object(arcT, posT+vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::Event_NextSong, {"next_song"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20,           0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_Mute, {"mute"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x,   0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_VolumeDown, {"vol_down"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x*2, 0), ssizT, FIX_Y | FIX_SIZ), &Program::Event_VolumeUp, {"vol_up"})
	};
}

ReaderBox::~ReaderBox() {
	clear(playerObjects);
	clear(listObjects);

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
	if (mouseHideable) {
		// handle mouse hide timeout
		if (World::scene()->FocusedObject() == this)
			if (mouseTimer != 0.f) {
				mouseTimer -= dSec;
				if (mouseTimer <= 0.f) {
					mouseTimer = 0.f;
					World::winSys()->ShowMouse(false);
				}
			}

		// handle slider hide timeout
		if (sliderTimer != 0.f && !sliderFocused) {
			sliderTimer -= World::engine()->deltaSeconds();
			if (sliderTimer <= 0.f) {
				sliderTimer = 0.f;
				World::engine()->SetRedrawNeeded();
			}
		}

		// handle list hide timeout
		if (listTimer != 0.f) {
			listTimer -= World::engine()->deltaSeconds();
			if (listTimer <= 0.f) {
				listTimer = 0.f;
				World::engine()->SetRedrawNeeded();
			}
		}

		// handle player hide timeout
		if (playerTimer != 0.f) {
			playerTimer -= World::engine()->deltaSeconds();
			if (playerTimer <= 0.f) {
				playerTimer = 0.f;
				World::engine()->SetRedrawNeeded();
			}
		}
	}
}

void ReaderBox::OnMouseMove(const vec2i& mPos) {
	// make sure mouse is shown
	if (mouseTimer != hideDelay) {
		mouseTimer = hideDelay;
		World::winSys()->ShowMouse(true);
	}

	// check where mouse is
	if (!CheckMouseOverSlider(mPos))
		if (!CheckMouseOverList(mPos))
			if (!CheckMouseOverPlayer(mPos))
				mouseHideable = true;
}

bool ReaderBox::CheckMouseOverSlider(const vec2i& mPos) {
	if (inRect(Bar(), mPos)) {
		if (sliderTimer != hideDelay) {
			sliderTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
}

bool ReaderBox::CheckMouseOverList(const vec2i& mPos) {
	SDL_Rect rect = List();
	if ((showList() && inRect(rect, mPos)) || (!showList() && inRect({rect.x, rect.y, rect.w/3, rect.h}, mPos))) {
		if (listTimer != hideDelay) {
			listTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
}

bool ReaderBox::CheckMouseOverPlayer(const vec2i& mPos) {
	SDL_Rect rect = Player();
	if (World::audioSys()->PlaylistLoaded() && ((showPlayer() && inRect(rect, mPos)) || (!showPlayer() && inRect({rect.x, rect.y+rect.h-rect.h/3, rect.w, rect.h/3}, mPos)))) {
		if (playerTimer != hideDelay) {
			playerTimer = hideDelay;
			World::engine()->SetRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
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

SDL_Rect ReaderBox::List() const {
	vec2i pos = Pos();
	return { pos.x, pos.y, blistW, int(listObjects.size())*blistW };
}

vector<Object*>& ReaderBox::ListObjects() {
	return listObjects;
}

SDL_Rect ReaderBox::Player() const {
	return { Pos().x+Size().x/2-playerW/2, End().y-playerH, playerW, playerH };
}

vector<Object*>& ReaderBox::PlayerObjects() {
	static_cast<Label*>(playerObjects[0])->text = World::audioSys()->curSongName();
	return playerObjects;
}

Image ReaderBox::getImage(size_t id) const {
	SDL_Rect crop;
	return getImage(id, crop);
}

Image ReaderBox::getImage(size_t id, SDL_Rect& crop) const {
	vec2i pos = Pos();
	Image img = pics[id];
	img.size = vec2i(pics[id].size.x*zoom, pics[id].size.y*zoom);
	img.pos = vec2i(pos.x + Size().x/2 - img.size.x/2 - listX, pics[id].pos.y*zoom - listY);
	
	crop = getCrop(img.getRect(), getRect());
	return img;
}

vec2t ReaderBox::VisibleItems() const {
	int sizY = Size().y;
	if (sizY <= 0)
		return vec2t(1, 0);

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
