#pragma once

#include "types.h"

class ListItem {
public:
	ListItem(string LBL="");
	virtual ~ListItem();
	
	virtual void OnClick();

	string label;
};

class BrowserButton : public ListItem {
public:
	BrowserButton(string LBL="", string DAT="", void (Program::*CALLB)(string)=nullptr);
	virtual ~BrowserButton();

	virtual void OnClick();
	void Callback(void (Program::*CALLB)(string));

	string data;
protected:
	void (Program::*callback)(string);
};

class TileItem {
public:
	TileItem(string LBL = "", string DAT = "", void (Program::*CALLB)(string) = nullptr);
	~TileItem();

	void OnClick();
	void Callback(void (Program::*CALLB)(string));

	string label;
	string data;

private:
	void (Program::*callback)(string);

};
