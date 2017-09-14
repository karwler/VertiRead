#include "engine/world.h"
#include "engine/filer.h"

// SCROLL AREA

ScrollArea::ScrollArea(const Widget& BASE) :
	Widget(BASE),
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

	return (lstH <= sizY) ? pos().y : pos().y + listY * sizY / lstH;
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

// SCROLL AREA ITEMS

ScrollAreaItems::ScrollAreaItems(const Widget& BASE) :
	ScrollArea(BASE),
	selectedItem(nullptr)
{}
ScrollAreaItems::~ScrollAreaItems() {}

void ScrollAreaItems::setValues(size_t numRows) {
	listH = numRows * (Default::itemHeight + Default::itemSpacing) - Default::itemSpacing;
	ScrollArea::setValues();
}

// LIST BOX

ListBox::ListBox(const Widget& BASE, const vector<ListItem*>& ITMS) :
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

void ListBox::setItems(const vector<ListItem*>& widgets) {
	clear(items);
	items = widgets;
	setValues();
}

ListItem* ListBox::item(size_t id) const {
	return items[id];
}

SDL_Rect ListBox::itemRect(size_t id) const {
	vec2i ps = pos();
	return {ps.x, ps.y - listY + id * (Default::itemHeight + Default::itemSpacing), size().x-barW(), Default::itemHeight};
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

TableBox::TableBox(const Widget& BASE, const grid2<ListItem*>& ITMS, const vector<float>& IWS) :
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

void TableBox::setItems(const grid2<ListItem*>& widgets) {
	clear(items);
	items = widgets;
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
	return {ps.x + pref*float(siz.x) + Default::itemSpacing/2, ps.y - listY + loc.y * (Default::itemHeight + Default::itemSpacing), itemW[loc.x]*float(siz.x) - Default::itemSpacing, Default::itemHeight};
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

TileBox::TileBox(const Widget& BASE, const vector<ListItem*>& ITMS, const vec2i& TS) :
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

void TileBox::setItems(const vector<ListItem*>& widgets) {
	clear(items);
	items = widgets;
	setValues();
}

ListItem* TileBox::item(size_t id) const {
	return items[id];
}

SDL_Rect TileBox::itemRect(size_t id) const {
	vec2i ps = pos();
	return {(id - (id/dim.x) * dim.x) * (tileSize.x+Default::itemSpacing) + ps.x, (id/dim.x) * (tileSize.y+Default::itemSpacing) + ps.y - listY, tileSize.x, tileSize.y};
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

ReaderBox::ReaderBox(const Widget& BASE, const string& DIR, const string& CPIC, float ZOOM) :
	ScrollArea(BASE),
	mouseHideable(true),
	mouseTimer(Default::rbMenuHideTimeout), sliderTimer(0.f), listTimer(0.f), playerTimer(0.f),
	zoom(ZOOM)
{
	setPictures(DIR, CPIC);

	vec2i arcT(pos());
	vec2i posT(-1);
	vec2i sizT(Default::rbBlistW);
	listWidgets = {
		new ButtonImage(Widget(arcT,                    posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventNextDir, {"next_dir.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x),   posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventPrevDir, {"prev_dir.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x*2), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomIn, {"zoom_in.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x*3), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomOut, {"zoom_out.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x*4), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventZoomReset, {"zoom_r.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x*5), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventCenterView, {"center.png"}),
		new ButtonImage(Widget(arcT+vec2i(0, sizT.x*6), posT, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, {"back.png"})
	};

	SDL_Rect player = playerRect();
	arcT += vec2i(size().x/2, size().y);
	posT = vec2i(arcT.x-sizT.x/2, arcT.y-sizT.y);
	vec2i ssizT(32);
	vec2i sposT(posT.x, arcT.y-Default::rbBlistW/2-ssizT.y/2);
	playerWidgets = {
		new Label(Widget(arcT, vec2i(player.x, player.y), vec2i(player.w, 20), FIX_Y | FIX_SIZ, EColor::darkened), "", ETextAlign::center),
		new ButtonImage(Widget(arcT, posT,                    sizT, FIX_Y | FIX_SIZ), &Program::eventPlayPause, {"play.png", "pause.png"}),
		new ButtonImage(Widget(arcT, posT-vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::eventPrevSong, {"prev_song.png"}),
		new ButtonImage(Widget(arcT, posT+vec2i(sizT.x, 0),   sizT, FIX_Y | FIX_SIZ), &Program::eventNextSong, {"next_song.png"}),
		new ButtonImage(Widget(arcT, sposT+vec2i(sizT.x*2+20,           0), ssizT, FIX_Y | FIX_SIZ), &Program::eventMute, {"mute.png"}),
		new ButtonImage(Widget(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x,   0), ssizT, FIX_Y | FIX_SIZ), &Program::eventVolumeDown, {"vol_down.png"}),
		new ButtonImage(Widget(arcT, sposT+vec2i(sizT.x*2+20+ssizT.x*2, 0), ssizT, FIX_Y | FIX_SIZ), &Program::eventVolumeUp, {"vol_up.png"})
	};
}

ReaderBox::~ReaderBox() {
	// save last page
	vec2t interval = visiblePictures();
	if (interval.x <= interval.y)
		Filer::saveLastPage(pics[interval.x].tex.getFile());

	// free up memory
	clearPictures();
	clear(playerWidgets);
	clear(listWidgets);

	// make cursor visible again
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
		if (World::scene()->getFocWidget() == this)
			if (mouseTimer != 0.f) {
				mouseTimer -= dSec;
				if (mouseTimer <= 0.f) {
					mouseTimer = 0.f;
					World::winSys()->setShowMouse(false);
				}
			}

		// handle slider hide timeout
		if (sliderTimer != 0.f && World::scene()->isDraggingSlider(this)) {
			sliderTimer -= World::base()->getDSec();
			if (sliderTimer <= 0.f) {
				sliderTimer = 0.f;
				World::winSys()->setRedrawNeeded();
			}
		}

		// handle list hide timeout
		if (listTimer != 0.f) {
			listTimer -= World::base()->getDSec();
			if (listTimer <= 0.f) {
				listTimer = 0.f;
				World::winSys()->setRedrawNeeded();
			}
		}

		// handle player hide timeout
		if (playerTimer != 0.f) {
			playerTimer -= World::base()->getDSec();
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

size_t ReaderBox::numPics() const {
	return pics.size();
}

void ReaderBox::checkListX() {
	if (listX < -listXL)
		listX = -listXL;
	else if (listX > listXL)
		listX = listXL;
}

void ReaderBox::setPictures(const string& dir, const string& curPic) {
	clearPictures();
	for (string& it : Filer::listDir(dir, FTYPE_FILE)) {
		Picture pic(listH, dir+it);
		if (!pic.tex.tex)
			continue;

		pics.push_back(pic);
		if (pic.tex.getFile() == curPic)
			listY = listH;
		if (pic.tex.getRes().x > listW)
			listW = pic.tex.getRes().x;
		listH += pic.tex.getRes().y + Default::itemSpacing;
	}
	setValues();
}

void ReaderBox::clearPictures() {
	for (Picture& it : pics)
		it.tex.clear();
	pics.clear();

	listH = 0;
	listW = 0;
	listY = 0;
	listX = 0;
}

SDL_Rect ReaderBox::listRect() const {
	vec2i ps = pos();
	return {ps.x, ps.y, Default::rbBlistW, int(listWidgets.size())*Default::rbBlistW};
}

const vector<Widget*>& ReaderBox::getListWidgets() const {
	return listWidgets;
}

SDL_Rect ReaderBox::playerRect() const {
	return {pos().x+size().x/2-Default::rbPlayerW/2, end().y-Default::rbPlayerH, Default::rbPlayerW, Default::rbPlayerH};
}

const vector<Widget*>& ReaderBox::getPlayerWidgets() {
	static_cast<Label*>(playerWidgets[0])->label = World::audioSys()->curSongName();
	return playerWidgets;
}

Texture* ReaderBox::texture(size_t id) {
	return &pics[id].tex;
}

Image ReaderBox::image(size_t id) {
	Image img(vec2i(0, pics[id].pos*zoom - listY), &pics[id].tex);
	img.size *= zoom;
	img.pos.x = pos().x + size().x/2 - img.size.x/2 - listX;
	return img;
}

vec2t ReaderBox::visiblePictures() const {
	int sizY = size().y;
	if (sizY <= 0)
		return vec2t(1, 0);

	vec2t interval(0, pics.size()-1);
	for (size_t i=interval.x; i<=interval.y; i++)
		if ((pics[i].pos + pics[i].tex.getRes().y)*zoom >= listY) {
			interval.x = i;
			break;
		}
	for (size_t i=interval.x; i<=interval.y; i++)
		if (pics[i].pos*zoom > listY + sizY) {
			interval.y = i-1;
			break;
		}
	return interval;
}

// READER BOX PICTURE

ReaderBox::Picture::Picture() :
	pos(0)
{}

ReaderBox::Picture::Picture(int POS, const string& file) :
	pos(POS),
	tex(file)
{}
