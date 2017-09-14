#pragma once

#include "types.h"

// item/element for ScrollAreas
class ListItem {
public:
	ListItem(const string& LBL="", ScrollAreaItems* SA=nullptr);
	virtual ~ListItem();
	
	virtual void onClick(ClickType click);
	virtual const string& getData() const;
	bool selectable() const;

	string label;
protected:
	ScrollAreaItems* parent;	// nullptr means "not selectable" (Only for this and ItemButton. Other classes derived from this one need to have a parent which has to be a ListBox.)
};

// clickable item with additional optional data
class ItemButton : public ListItem {
public:
	ItemButton(const string& LBL="", const string& DAT="", void (Program::*LCALL)(void*)=nullptr, void (Program::*RCALL)(void*)=nullptr, ScrollAreaItems* SA=nullptr);
	virtual ~ItemButton();

	virtual void onClick(ClickType click);
	virtual const string& getData() const;		// returns either label or data
	void setData(const string& dat);

private:
	void (Program::*lcall)(void*);	// on left click
	void (Program::*rcall)(void*);	// on right click
	string data;	// additional text data that might be needed for other operations when the button is pressed (in case label isn't enough)
};

// the most basic checkbox
class Checkbox : public ListItem {
public:
	Checkbox(ScrollAreaItems* SA, const string& LBL="", bool ON=false, void (Program::*CALL)(bool)=nullptr);
	virtual ~Checkbox();

	virtual void onClick(ClickType click);
	bool getOn() const;
	void setOn(bool ON);

private:
	void (Program::*call)(bool);
	bool on;
};

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class Switchbox : public ListItem {
public:
	Switchbox(ScrollAreaItems* SA, const string& LBL="", const vector<string>& OPT={}, const string& CUR_OPT="", void (Program::*CALL)(const string&)=nullptr);
	virtual ~Switchbox();

	virtual void onClick(ClickType click);
	string getCurOption() const;

private:
	void (Program::*call)(const string&);
	size_t curOpt;			// index of the currently selected/displayed option
	vector<string> options;	// all the possible things one can choose
};
