#include "engine/world.h"

// SCROLL AREA

ScrollArea::ScrollArea(const Object& BASE) :
	Object(BASE),
	selectedItem(nullptr),
	diffSliderMouseY(0),
	listY(0),
	motion(0.f)
{}
ScrollArea::~ScrollArea() {}

void ScrollArea::dragSlider(int ypos) {
	dragList((ypos - diffSliderMouseY - pos().y) * listH / size().y);
}

void ScrollArea::dragList(int ypos) {
	listY = ypos;
	checkListY();
	World::winSys()->setRedrawNeeded();
}

void ScrollArea::scrollList(int ymov) {
	dragList(listY + ymov);
}

void ScrollArea::tick(float dSec) {
	if (motion != 0.f) {
		scrollList(motion);

		motion = (motion > 0.f) ? motion - dSec * Default::scrollThrottle : motion + dSec * Default::scrollThrottle;
		if (std::abs(motion) < 1.f)
			motion = 0.f;
	}
}

void ScrollArea::setValues() {
	int sizY = size().y;
	int lstH = getListH();

	listL = (sizY < lstH) ? lstH - sizY : 0;
	sliderH = (sizY < lstH) ? sizY * sizY / lstH : sizY;
	checkListY();
}

void ScrollArea::setMotion(float val) {
	motion = val;
}

int ScrollArea::barW() const {
	return (sliderH == size().y) ? 0 : Default::scrollBarWidth;
}

int ScrollArea::getListY() const {
	return listY;
}

int ScrollArea::getListH() const {
	return listH;
}

float ScrollArea::getZoom() const {
	return 1.f;
}

int ScrollArea::getListL() const {
	return listL;
}

int ScrollArea::sliderY() const {
	int sizY = size().y;
	int lstH = getListH();

	return lstH <= sizY ? pos().y : pos().y + listY * sizY / lstH;
}

int ScrollArea::getSliderH() const {
	return sliderH;
}

void ScrollArea::checkListY() {
	if (listY < 0)
		listY = 0;
	else if (listY > listL)
		listY = listL;
}

SDL_Rect ScrollArea::barRect() const {
	int bw = barW();
	return {end().x-bw, pos().y, bw, size().y};
}

SDL_Rect ScrollArea::sliderRect() const {
	int bw = barW();
	return {end().x-bw, sliderY(), bw, sliderH};
}

btsel ScrollArea::getSelectedItem() const {
	return btsel();
}

vec2t ScrollArea::visibleItems() const {
	return vec2t(1, 0);
}

// SCROLL AREA X1

ScrollAreaItems::ScrollAreaItems(const Object& BASE) :
	ScrollArea(BASE)
{}
ScrollAreaItems::~ScrollAreaItems() {}

void ScrollAreaItems::setValues(size_t numRows) {
	listH = numRows * (Default::itemHeight + Default::itemSpacing) - Default::itemSpacing;
	ScrollArea::setValues();
}

// LIST BOX

ListBox::ListBox(const Object& BASE, const vector<ListItem*>& ITMS) :
	ScrollAreaItems(BASE)
{
	if (!ITMS.empty())
		setItems(ITMS);
}

ListBox::~ListBox() {
	clear(items);
}

ListBox* ListBox::clone() const {
	return new ListBox(*this);
}

void ListBox::setValues() {
	ScrollAreaItems::setValues(items.size());
}

vector<ListItem*> ListBox::getItems() const {
	return items;
}

void ListBox::setItems(const vector<ListItem*>& objects) {
	clear(items);
	items = objects;
	setValues();
}

ListItem* ListBox::item(size_t id) const {
	return items[id];
}

SDL_Rect ListBox::itemRect(size_t id) const {
	vec2i ps = pos();
	SDL_Rect rct = {ps.x, ps.y - listY + id * (Default::itemHeight + Default::itemSpacing), size().x-barW(), Default::itemHeight};
	return cropRect(rct, getCrop(rct, rect()));
}

SDL_Rect ListBox::itemRect(size_t id, SDL_Rect& crop, EColor& color) const {
	vec2i ps = pos();
	SDL_Rect rct = {ps.x, ps.y - listY + id * (Default::itemHeight + Default::itemSpacing), size().x-barW(), Default::itemHeight};

	color = items[id] == selectedItem ? EColor::highlighted : EColor::rectangle;
	crop = getCrop(rct, rect());
	return cropRect(rct, crop);
}

btsel ListBox::getSelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t ListBox::visibleItems() const {
	int sizY = size().y;
	if (items.empty() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t(listY / (Default::itemHeight + Default::itemSpacing), (listH > sizY) ? (listY+sizY) / (Default::itemHeight + Default::itemSpacing) : items.size()-1);
}

// TABLE BOX

TableBox::TableBox(const Object& BASE, const grid2<ListItem*>& ITMS, const vector<float>& IWS) :
	ScrollAreaItems(BASE),
	itemW(IWS)
{
	if (ITMS.arr())
		setItems(ITMS);
}

TableBox::~TableBox() {
	clear(items);
}

TableBox* TableBox::clone() const {
	return new TableBox(*this);
}

void TableBox::setValues() {
	ScrollAreaItems::setValues(items.size().y);
}

vector<ListItem*> TableBox::getItems() const {
	return vector<ListItem*>(items.begin(), items.end());
}

const grid2<ListItem*>& TableBox::getItemsGrid() const {
	return items;
}

void TableBox::setItems(const grid2<ListItem*>& objects) {
	clear(items);
	items = objects;
	setValues();
}

ListItem* TableBox::item(size_t id) const {
	return items[id];
}

SDL_Rect TableBox::itemRect(size_t id) const {
	vec2i ps = pos();
	vec2i siz = size();
	vec2u loc = items.loc(id);
	float pref = 0.f;
	for (uint i=0; i!=loc.x; i++)
		pref += itemW[i];

	SDL_Rect rct = {ps.x + pref*float(siz.x) + Default::itemSpacing/2, ps.y - listY + loc.y * (Default::itemHeight + Default::itemSpacing), itemW[loc.x]*float(siz.x) - Default::itemSpacing, Default::itemHeight};
	return cropRect(rct, getCrop(rct, rect()));
}

SDL_Rect TableBox::itemRect(size_t id, SDL_Rect& crop, EColor& color) const {
	vec2i ps = pos();
	vec2i siz = size();
	vec2u loc = items.loc(id);
	float pref = 0.f;
	for (uint i=0; i!=loc.x; i++)
		pref += itemW[i];

	SDL_Rect rct = {ps.x + pref*float(siz.x) + Default::itemSpacing/2, ps.y - listY + loc.y * (Default::itemHeight + Default::itemSpacing), itemW[loc.x]*float(siz.x) - Default::itemSpacing, Default::itemHeight};
	color = EColor::rectangle;
	crop = getCrop(rct, rect());
	return cropRect(rct, crop);
}

btsel TableBox::getSelectedItem() const {
	for (uint i=0; i!=items.length(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t TableBox::visibleItems() const {
	int sizY = size().y;
	if (!items.arr() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t((listY / (Default::itemHeight + Default::itemSpacing)) * items.size().x, (listH > sizY) ? ((listY+sizY) / (Default::itemHeight + Default::itemSpacing)) * items.size().x + items.size().x-1 : items.length()-1);
}

// TILE BOX

TileBox::TileBox(const Object& BASE, const vector<ListItem*>& ITMS, const vec2i& TS) :
	ScrollAreaItems(BASE),
	tileSize(TS)
{
	if (!ITMS.empty())
		setItems(ITMS);
}

TileBox::~TileBox() {
	clear(items);
}

TileBox* TileBox::clone() const {
	return new TileBox(*this);
}

void TileBox::setValues() {
	int sizX = size().x;

	dim.x = (sizX - Default::scrollBarWidth > tileSize.x + Default::itemSpacing) ? (sizX - Default::scrollBarWidth) / (tileSize.x + Default::itemSpacing) : 1;	// column count
	dim.y = (items.size() > dim.x) ? items.size() / dim.x : 1;																									// row count
	ScrollAreaItems::setValues(dim.y);
}

vec2i TileBox::getTileSize() const {
	return tileSize;
}

vec2i TileBox::getDim() const {
	return dim;
}

vector<ListItem*> TileBox::getItems() const {
	return items;
}

void TileBox::setItems(const vector<ListItem*>& objects) {
	clear(items);
	items = objects;
	setValues();
}

ListItem* TileBox::item(size_t id) const {
	return items[id];
}

SDL_Rect TileBox::itemRect(size_t id) const {
	vec2i ps = pos();
	SDL_Rect rct = {(id - (id/dim.x) * dim.x) * (tileSize.x+Default::itemSpacing) + ps.x, (id/dim.x) * (tileSize.y+Default::itemSpacing) + ps.y - listY, tileSize.x, tileSize.y};
	return cropRect(rct, getCrop(rct, rect()));
}

SDL_Rect TileBox::itemRect(size_t id, SDL_Rect& crop, EColor& color) const {
	vec2i ps = pos();
	SDL_Rect rct = {(id - (id/dim.x) * dim.x) * (tileSize.x+Default::itemSpacing) + ps.x, (id/dim.x) * (tileSize.y+Default::itemSpacing) + ps.y - listY, tileSize.x, tileSize.y};

	color = (items[id] == selectedItem) ? EColor::highlighted : EColor::rectangle;
	crop = getCrop(rct, rect());
	return cropRect(rct, crop);
}

btsel TileBox::getSelectedItem() const {
	for (size_t i=0; i!=items.size(); i++)
		if (items[i] == selectedItem)
			return i;
	return btsel();
}

vec2t TileBox::visibleItems() const {
	int sizY = size().y;
	if (items.empty() || sizY <= 0)
		return vec2t(1, 0);
	return vec2t(listY / (tileSize.y+Default::itemSpacing) * dim.x, (listH > sizY) ? (listY+sizY) / (tileSize.y+Default::itemSpacing) * dim.x + dim.x-1 : items.size()-1);
}

// READER BOX

ReaderBox::ReaderBox(const Object& BASE, const vector<Texture*>& PICS, const string& CURPIC, float ZOOM) :
	ScrollArea(BASE),
	mouseHideable(true),
	mouseTimer(Default::rbMenuHideTimeout), sliderTimer(0.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM),
	listX(0)
{
	if (!PICS.empty())
		setPictures(PICS, CURPIC);

	vec2i arcT(pos());
	vec2i posT(-1);
	vec2i sizT(Default::rbBlistW);
	listObjects = {
		new ButtonImage(Object(arcT,                    posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventNextDir, {"next_dir"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x),   posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventPrevDir, {"prev_dir"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*2), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomIn, {"zoom_in"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*3), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomOut, {"zoom_out"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*4), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomReset, {"zoom_r"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*5), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventCenterView, {"center"}),
		new ButtonImage(Object(arcT+vec2i(0, sizT.x*6), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, {"back"})
	};

	SDL_Rect player = playerRect();
	arcT += vec2i(size().x/2, size().y);
	posT = vec2i(arcT.x-sizT.x/2, arcT.y-sizT.y);
	vec2i ssizT(32);
	vec2i sposT(posT.x, arcT.y-Default::rbBlistW/2-ssizT.y/2);
	playerObjects = {
		new Label(Object(arcT, vec2i(player.x, player.y), vec2i(player.w, 20), FIX_Y | FIX_SIZ, EColor::darkened), "", ETextAlign::center),
		new ButtonImage(Object(arcT, posT,                    sizT, FIX_Y | FIX_SIZ), &Program::eventPlayPause, {"play", "pause"}),
		new ButtonImage(Object(arcT, posT-vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::eventPrevSong, {"prev_song"}),
		new ButtonImage(Object(arcT, posT+vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::eventNextSong, {"next_song"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20,           0), ssizT, FIX_Y | FIX_SIZ), &Program::eventMute, {"mute"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x,   0), ssizT, FIX_Y | FIX_SIZ), &Program::eventVolumeDown, {"vol_down"}),
		new ButtonImage(Object(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x*2, 0), ssizT, FIX_Y | FIX_SIZ), &Program::eventVolumeUp, {"vol_up"})
	};
}

ReaderBox::~ReaderBox() {
	clear(playerObjects);
	clear(listObjects);

	if (!World::winSys()->getShowMouse())
		World::winSys()->setShowMouse(true);
}

ReaderBox* ReaderBox::clone() const {
	return new ReaderBox(*this);
}

void ReaderBox::tick(float dSec) {
	ScrollArea::tick(dSec);

	if (mouseHideable) {
		// handle mouse hide timeout
		if (World::scene()->getFocObject() == this)
			if (mouseTimer != 0.f) {
				mouseTimer -= dSec;
				if (mouseTimer <= 0.f) {
					mouseTimer = 0.f;
					World::winSys()->setShowMouse(false);
				}
			}

		// handle slider hide timeout
		if (sliderTimer != 0.f && World::scene()->isDraggingSlider(this)) {
			sliderTimer -= World::engine()->getDSec();
			if (sliderTimer <= 0.f) {
				sliderTimer = 0.f;
				World::winSys()->setRedrawNeeded();
			}
		}

		// handle list hide timeout
		if (listTimer != 0.f) {
			listTimer -= World::engine()->getDSec();
			if (listTimer <= 0.f) {
				listTimer = 0.f;
				World::winSys()->setRedrawNeeded();
			}
		}

		// handle player hide timeout
		if (playerTimer != 0.f) {
			playerTimer -= World::engine()->getDSec();
			if (playerTimer <= 0.f) {
				playerTimer = 0.f;
				World::winSys()->setRedrawNeeded();
			}
		}
	}
}

void ReaderBox::onMouseMove(const vec2i& mPos) {
	// make sure mouse is shown
	if (mouseTimer != Default::rbMenuHideTimeout) {
		mouseTimer = Default::rbMenuHideTimeout;
		World::winSys()->setShowMouse(true);
	}

	// check where mouse is
	if (!checkMouseOverSlider(mPos))
		if (!checkMouseOverList(mPos))
			if (!checkMouseOverPlayer(mPos))
				mouseHideable = true;
}

bool ReaderBox::checkMouseOverSlider(const vec2i& mPos) {
	if (inRect(barRect(), mPos)) {
		if (sliderTimer != Default::rbMenuHideTimeout) {
			sliderTimer = Default::rbMenuHideTimeout;
			World::winSys()->setRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
}

bool ReaderBox::checkMouseOverList(const vec2i& mPos) {
	SDL_Rect rect = listRect();
	if ((showList() && inRect(rect, mPos)) || (!showList() && inRect({rect.x, rect.y, rect.w/3, rect.h}, mPos))) {
		if (listTimer != Default::rbMenuHideTimeout) {
			listTimer = Default::rbMenuHideTimeout;
			World::winSys()->setRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
}

bool ReaderBox::checkMouseOverPlayer(const vec2i& mPos) {
	SDL_Rect rect = playerRect();
	if (World::audioSys()->playlistLoaded() && ((showPlayer() && inRect(rect, mPos)) || (!showPlayer() && inRect({rect.x, rect.y+rect.h-rect.h/3, rect.w, rect.h/3}, mPos)))) {
		if (playerTimer != Default::rbMenuHideTimeout) {
			playerTimer = Default::rbMenuHideTimeout;
			World::winSys()->setRedrawNeeded();
		}
		mouseHideable = false;
		return true;
	}
	return false;
}

void ReaderBox::dragListX(int xpos) {
	listX = xpos;
	checkListX();
	World::winSys()->setRedrawNeeded();
}

void ReaderBox::scrollListX(int xmov) {
	dragListX(listX + xmov);
}

void ReaderBox::setZoom(float factor) {
	// correct xy position
	listY = float(listY) * factor / zoom;
	listX = float(listX) * factor / zoom;
	zoom = factor;
	setValues();

	World::winSys()->setRedrawNeeded();
}

void ReaderBox::multZoom(float zfactor) {
	setZoom(zoom * zfactor);
}

void ReaderBox::divZoom(float zfactor) {
	setZoom(zoom / zfactor);
}

void ReaderBox::setValues() {
	listXL = (size().x < getListW()) ? (getListW() - size().x) /2 : 0;
	checkListX();
	ScrollArea::setValues();
}

int ReaderBox::getListX() const {
	return listX;
}

int ReaderBox::getListW() const {
	return listW * zoom;
}

int ReaderBox::getListH() const {
	return listH * zoom;
}

float ReaderBox::getZoom() const {
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

void ReaderBox::checkListX() {
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
}

const vector<Image>& ReaderBox::getPictures() const {
	return pics;
}

void ReaderBox::setPictures(const vector<Texture*>& pictures, const string& curPic) {
	listH = 0;
	listW = 0;
	listY = 0;
	for (Texture* it : pictures) {
		pics.push_back(Image(vec2i(0, listH), it));
		if (it->getFile() == curPic)
			listY = listH;
		if (it->resolution().x > listW)
			listW = it->resolution().x;
		listH += it->resolution().y + Default::itemSpacing;
	}
	setValues();
}

SDL_Rect ReaderBox::listRect() const {
	vec2i ps = pos();
	return {ps.x, ps.y, Default::rbBlistW, int(listObjects.size())*Default::rbBlistW};
}

const vector<Object*>& ReaderBox::getListObjects() const {
	return listObjects;
}

SDL_Rect ReaderBox::playerRect() const {
	return {pos().x+size().x/2-Default::rbPlayerW/2, end().y-Default::rbPlayerH, Default::rbPlayerW, Default::rbPlayerH};
}

const vector<Object*>& ReaderBox::getPlayerObjects() {
	static_cast<Label*>(playerObjects[0])->text = World::audioSys()->curSongName();
	return playerObjects;
}

Image ReaderBox::image(size_t id) const {
	SDL_Rect crop;
	return image(id, crop);
}

Image ReaderBox::image(size_t id, SDL_Rect& crop) const {
	Image img = pics[id];
	img.size = vec2i(pics[id].size.x*zoom, pics[id].size.y*zoom);
	img.pos = vec2i(pos().x + size().x/2 - img.size.x/2 - listX, pics[id].pos.y*zoom - listY);
	
	crop = getCrop(img.rect(), rect());
	return img;
}

vec2t ReaderBox::visibleItems() const {
	int sizY = size().y;
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
