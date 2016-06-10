#pragma once

#include "types.h"

class ListItem {
public:
	ListItem(const string& LBL="", ScrollArea* SA=nullptr);
	virtual ~ListItem();
	
	virtual void OnClick(EClick clickType);
	bool selectable() const;

	string label;
protected:
	ScrollArea* parent;	// nullptr means "not selectable" (Only for this and ItemButton. Other classes derived from this one need to have a parent which has to be a ListBox.)
};

class ItemButton : public ListItem {
public:
	ItemButton(const string& LBL="", const string& DAT="", void (Program::*CALLB)(void*)=nullptr, ScrollArea* SA=nullptr);
	virtual ~ItemButton();

	virtual void OnClick(EClick clickType);

	string data;
private:
	void (Program::*callback)(void*);
};

class Checkbox : public ListItem {
public:
	Checkbox(ListBox* SA, const string& LBL="", bool ON=false, void (Program::*CALLB)(bool)=nullptr, int SPC=5);
	virtual ~Checkbox();

	virtual void OnClick(EClick clickType);
	ListBox* Parent() const;
	bool On() const;

	const int spacing;
private:
	void (Program::*callback)(bool);
	bool on;
};

class Switchbox : public ListItem {
public:
	Switchbox(ListBox* SA, const string& LBL="", const vector<string>& OPT={}, const string& CUR_OPT="", void (Program::*CALLB)(const string&)=nullptr);
	virtual ~Switchbox();

	virtual void OnClick(EClick clickType);
	ListBox* Parent() const;
	string CurOption() const;

private:
	void (Program::*callback)(const string&);
	uint curOpt;
	vector<string> options;
};