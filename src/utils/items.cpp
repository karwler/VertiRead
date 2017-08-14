#include "engine/world.h"

// LIST ITEM

ListItem::ListItem(const string& LBL, ScrollAreaItems* SA) :
	label(LBL),
	parent(SA)
{}
ListItem::~ListItem() {}

void ListItem::onClick(ClickType click) {
	if (click.button == SDL_BUTTON_LEFT) {
		if (parent)		// if item is selectable, select it
			parent->selectedItem = this;
		if (click.clicks == 2)	// double click event
			World::program()->eventItemDoubleclicked(this);
	}
	World::winSys()->setRedrawNeeded();
}

const string& ListItem::getData() const {
	return label;
}

bool ListItem::selectable() const {
	return parent != nullptr;
}

// ITEM BUTTON

ItemButton::ItemButton(const string& LBL, const string& DAT, void (Program::*CALLB)(void*), ScrollAreaItems* SA) :
	ListItem(LBL, SA),
	call(CALLB),
	data(DAT)
{}
ItemButton::~ItemButton() {}

void ItemButton::onClick(ClickType click) {
	ListItem::onClick(click);

	// decide what to send
	void* dat;
	if (parent)
		dat = parent;
	else
		dat = (void*)getData().c_str();
	
	if (call)
		(World::program()->*call)(dat);
}

const string& ItemButton::getData() const {
	return data.empty() ? label : data;
}

void ItemButton::setData(const string& dat) {
	data = dat;
}

// CHECKBOX

Checkbox::Checkbox(ScrollAreaItems* SA, const string& LBL, bool ON, void (Program::*CALLB)(bool)) :
	ListItem(LBL, SA),
	call(CALLB),
	on(ON)
{}
Checkbox::~Checkbox() {}

void Checkbox::onClick(ClickType click) {
	on = !on;
	if (call)
		(World::program()->*call)(on);

	World::winSys()->setRedrawNeeded();
}

bool Checkbox::getOn() const {
	return on;
}

void Checkbox::setOn(bool ON) {
	on = ON;
	World::winSys()->setRedrawNeeded();
}

// SWITCHBOX

Switchbox::Switchbox(ScrollAreaItems* SA, const string& LBL, const vector<string>& OPT, const string& CUR_OPT, void (Program::*CALLB)(const string&)) :
	ListItem(LBL, SA),
	call(CALLB),
	curOpt(0),
	options(OPT)
{
	if (options.empty())		// there needs to be at least one element
		options.push_back("");

	// set current option (remains 0 if nothing found)
	for (size_t i=0; i!=options.size(); i++)
		if (options[i] == CUR_OPT) {
			curOpt = i;
			break;
		}	
}
Switchbox::~Switchbox() {}

void Switchbox::onClick(ClickType click) {
	// left click cycles forward and right click cycles backward
	if (click.button == SDL_BUTTON_LEFT)
		curOpt = (curOpt == options.size()-1) ? 0 : curOpt + 1;
	else if (click.button == SDL_BUTTON_RIGHT)
		curOpt = (curOpt == 0) ? options.size()-1 : curOpt - 1;
	
	if (call)
		(World::program()->*call)(options[curOpt]);
}

string Switchbox::getCurOption() const {
	return options[curOpt];
}
