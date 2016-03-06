#include "items.h"
#include "engine/world.h"

// LIST ITEM

ListItem::ListItem(int H, string LBL) :
	height(H),
	label(LBL)
{}
ListItem::~ListItem() {}

void ListItem::OnClick() {

}

// BROWSER BUTTON

BrowserButton::BrowserButton(int H, string LBL, string DAT, void (Program::*CALLB)(string)) :
	ListItem(H, LBL),
	data(DAT),
	callback(CALLB)
{}
BrowserButton::~BrowserButton() {}

void BrowserButton::OnClick() {
	if (!callback)
		return;
	if (data.empty())
		(World::program()->*callback)(label);
	else
		(World::program()->*callback)(data);
}

void BrowserButton::setCallback(void(Program::* CALLB)(string)) {
	callback = CALLB;
}

// TILE ITEM

TileItem::TileItem(string LBL, string DAT, void (Program::*CALLB)(string)) :
	label(LBL),
	data(DAT),
	callback(CALLB)
{}
TileItem::~TileItem() {}

void TileItem::OnClick() {
	if (!callback)
		return;
	if (data.empty())
		(World::program()->*callback)(label);
	else
		(World::program()->*callback)(data);
}

void TileItem::setCallback(void (Program::*CALLB)(string)) {
	callback = CALLB;
}
