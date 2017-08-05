#include "engine/world.h"

// LIST ITEM

ListItem::ListItem(const string& LBL, ScrollArea* SA) :
	label(LBL),
	parent(SA)
{}
ListItem::~ListItem() {}

void ListItem::OnClick(ClickType click) {
	if (parent)
		parent->selectedItem = this;
	if (click.button == SDL_BUTTON_LEFT && click.clicks == 2)
		World::program()->Event_ItemDoubleclicked(this);

	World::winSys()->SetRedrawNeeded();
}

const string& ListItem::getData() const {
	return label;
}

bool ListItem::selectable() const {
	return parent != nullptr;
}

// ITEM BUTTON

ItemButton::ItemButton(const string& LBL, const string& DAT, void (Program::*CALLB)(void*), ScrollArea* SA) :
	ListItem(LBL, SA),
	callback(CALLB),
	data(DAT)
{}
ItemButton::~ItemButton() {}

void ItemButton::OnClick(ClickType click) {
	ListItem::OnClick(click);

	// decide what to send
	void* dat;
	if (parent)
		dat = parent;
	else
		dat = (void*)getData().c_str();
	
	if (callback)
		(World::program()->*callback)(dat);
}

const string& ItemButton::getData() const {
	return data.empty() ? label : data;
}

void ItemButton::Data(const string& dat) {
	data = dat;
}

// CHECKBOX

Checkbox::Checkbox(ScrollAreaX1* SA, const string& LBL, bool ON, void (Program::*CALLB)(bool)) :
	ListItem(LBL, SA),
	callback(CALLB),
	on(ON)
{}
Checkbox::~Checkbox() {}

void Checkbox::OnClick(ClickType click) {
	on = !on;
	if (callback)
		(World::program()->*callback)(on);

	World::winSys()->SetRedrawNeeded();
}

ScrollAreaX1* Checkbox::Parent() const {
	return static_cast<ScrollAreaX1*>(parent);
}

bool Checkbox::On() const {
	return on;
}

// SWITCHBOX

Switchbox::Switchbox(ScrollAreaX1* SA, const string& LBL, const vector<string>& OPT, const string& CUR_OPT, void (Program::*CALLB)(const string&)) :
	ListItem(LBL, SA),
	callback(CALLB),
	curOpt(0),
	options(OPT)
{
	if (options.empty())		// there needs to be at least one element
		options.push_back("");

	for (size_t i=0; i!=options.size(); i++)
		if (options[i] == CUR_OPT) {
			curOpt = i;
			break;
		}	
}
Switchbox::~Switchbox() {}

void Switchbox::OnClick(ClickType click) {
	if (click.button == SDL_BUTTON_LEFT && click.clicks == 2)
		curOpt = (curOpt == options.size()-1) ? 0 : curOpt + 1;
	else
		curOpt = (curOpt == 0) ? options.size()-1 : curOpt - 1;
	
	if (callback)
		(World::program()->*callback)(options[curOpt]);
}

ScrollAreaX1* Switchbox::Parent() const {
	return static_cast<ScrollAreaX1*>(parent);
}

string Switchbox::CurOption() const {
	return options[curOpt];
}
