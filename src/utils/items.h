#pragma once

#include "types.h"

class ListItem {
public:
	ListItem(const string& LBL="", ScrollArea* SA=nullptr);
	virtual ~ListItem();
	
	virtual void OnClick(bool doubleclick);
	bool selectable() const;

	string label;
protected:
	ScrollArea* parent;	// nullptr means "not selectable" (Only for this and ItemButton. Other classes derived from this one need to have a parent which has to be a ListBox.)
};

class ItemButton : public ListItem {
public:
	ItemButton(const string& LBL="", const string& DAT="", void (Program::*CALLB)(void*)=nullptr, ScrollArea* SA=nullptr);
	virtual ~ItemButton();

	virtual void OnClick(bool doubleclick);

	string data;
protected:
	void (Program::*callback)(void*);
};

class Checkbox : public ListItem {
public:
	Checkbox(ListBox* SA, const string& LBL="", bool ON=false, void (Program::*CALLB)(bool)=nullptr, int SPC=5);
	virtual ~Checkbox();

	virtual void OnClick(bool doubleclick);
	ListBox* Parent() const;
	bool On() const;

	const int spacing;
private:
	bool on;
	void (Program::*callback)(bool);
};