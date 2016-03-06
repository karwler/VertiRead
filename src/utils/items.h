#pragma once

#include "types.h"

class ListItem {
public:
	ListItem(int H=50, string LBL="");
	virtual ~ListItem();
	
	virtual void OnClick();

	int height;
	string label;
};

class BrowserButton : public ListItem {
public:
	BrowserButton(int H = 50, string LBL = "", string DAT="", void (Program::*CALLB)(string)=nullptr);
	virtual ~BrowserButton();

	virtual void OnClick();
	void setCallback(void (Program::*CALLB)(string));

	string data;
protected:
	void (Program::*callback)(string);
};

class TileItem {
public:
	TileItem(string LBL = "", string DAT = "", void (Program::*CALLB)(string) = nullptr);
	~TileItem();

	void OnClick();
	void setCallback(void (Program::*CALLB)(string));

	string label;
	string data;

private:
	void (Program::*callback)(string);

};
