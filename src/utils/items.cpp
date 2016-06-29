#include "engine/world.h"

// LIST ITEM

ListItem::ListItem(const string& LBL, ScrollArea* SA) :
	label(LBL),
	parent(SA)
{}
ListItem::~ListItem() {}

void ListItem::OnClick(EClick clickType) {
	if (parent)
		parent->selectedItem = this;
	if (clickType == EClick::left_double)
		World::program()->Event_ItemDoubleclicked(this);

	World::engine->SetRedrawNeeded();
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
	data(DAT),
	callback(CALLB)
{}
ItemButton::~ItemButton() {}

void ItemButton::OnClick(EClick clickType) {
	ListItem::OnClick(clickType);

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

Checkbox::Checkbox(ListBox* SA, const string& LBL, bool ON, void (Program::*CALLB)(bool), int SPC) :
	ListItem(LBL, SA),
	callback(CALLB),
	on(ON),
	spacing(SPC)
{}
Checkbox::~Checkbox() {}

void Checkbox::OnClick(EClick clickType) {
	on = !on;
	if (callback)
		(World::program()->*callback)(on);

	World::engine->SetRedrawNeeded();
}

ListBox* Checkbox::Parent() const {
	return static_cast<ListBox*>(parent);
}

bool Checkbox::On() const {
	return on;
}

// SWITCHBOX

Switchbox::Switchbox(ListBox* SA, const string& LBL, const vector<string>& OPT, const string& CUR_OPT, void (Program::*CALLB)(const string&)) :
	ListItem(LBL, SA),
	callback(CALLB),
	curOpt(0),
	options(OPT)
{
	if (options.empty())		// there needs to be at least one element
		options.push_back("");

	for (uint i=0; i!=options.size(); i++)
		if (options[i] == CUR_OPT) {
			curOpt = i;
			break;
		}	
}
Switchbox::~Switchbox() {}

void Switchbox::OnClick(EClick clickType) {
	if (clickType == EClick::left || clickType == EClick::left_double)
		curOpt = (curOpt == options.size()-1) ? 0 : curOpt + 1;
	else
		curOpt = (curOpt == 0) ? options.size()-1 : curOpt - 1;
	
	if (callback)
		(World::program()->*callback)(options[curOpt]);
}

ListBox* Switchbox::Parent() const {
	return static_cast<ListBox*>(parent);
}

string Switchbox::CurOption() const {
	return options[curOpt];
}
