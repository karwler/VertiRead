#include "engine/world.h"

// LIST ITEM

ListItem::ListItem(const string& LBL, ScrollArea* SA) :
	label(LBL),
	parent(SA)
{}
ListItem::~ListItem() {}

void ListItem::OnClick(bool doubleclick) {
	if (doubleclick)
		World::program()->Event_ItemDoubleclicked(this);
	else if (parent)
		parent->selectedItem = this;
	World::engine->SetRedrawNeeded();
}

bool ListItem::selectable() const {
	return parent != nullptr;
}

// ITEM BUTTON

ItemButton::ItemButton(const string& LBL, const string& DAT, void (Program::*CALLB)(void*), ScrollArea* SA) :
	ListItem(LBL, SA),
	data(DAT),
	callback(CALLB)
{}
ItemButton::~ItemButton() {}

void ItemButton::OnClick(bool doubleclick) {
	ListItem::OnClick(doubleclick);

	void* dat;	// decide what to send
	if (parent)
		dat = parent;
	else if (data.empty())
		dat = (void*)label.c_str();
	else
		dat = (void*)data.c_str();

	if (callback)
		(World::program()->*callback)(dat);
}
