#pragma once

#include "types.h"

class ListItem {
public:
	ListItem(const string& LBL="", ScrollArea* SA=nullptr);
	virtual ~ListItem();
	
	virtual void OnClick(EClick clickType);
	virtual const string& getData() const;
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
	virtual const string& getData() const;		// returns either label or data
	void Data(const string& dat);

private:
	void (Program::*callback)(void*);
	string data;
};

class Checkbox : public ListItem {
public:
	Checkbox(ScrollAreaX1* SA, const string& LBL="", bool ON=false, void (Program::*CALLB)(bool)=nullptr, int SPC=5);
	virtual ~Checkbox();

	virtual void OnClick(EClick clickType);
	ScrollAreaX1* Parent() const;
	bool On() const;

	const int spacing;
private:
	void (Program::*callback)(bool);
	bool on;
};

class Switchbox : public ListItem {
public:
	Switchbox(ScrollAreaX1* SA, const string& LBL="", const vector<string>& OPT={}, const string& CUR_OPT="", void (Program::*CALLB)(const string&)=nullptr);
	virtual ~Switchbox();

	virtual void OnClick(EClick clickType);
	ScrollAreaX1* Parent() const;
	string CurOption() const;

private:
	void (Program::*callback)(const string&);
	size_t curOpt;
	vector<string> options;
};
