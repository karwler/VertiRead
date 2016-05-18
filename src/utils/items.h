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
	ScrollArea* parent;	// nullptr means "not selectable"
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
