#pragma once

#include "capturers.h"

class Popup : public Object {
public:
	Popup(const vec2i& SIZ=vec2i(500, 200), float TO=0.f);
	virtual ~Popup();
	virtual Popup* Clone() const;

	void Tick();
	virtual vector<Object*> getObjects();

protected:
	float timeout;	// 0 means no timeout
};

class PopupMessage : public Popup {
public:
	PopupMessage(const string& MSG="", int W=500, int TH=48, int BH=32, float TO=0.f);
	virtual ~PopupMessage();
	virtual PopupMessage* Clone() const;

	virtual vector<Object*> getObjects();
	SDL_Rect CancelButton() const;

protected:
	Label title;
	Label cButton;
};

class PopupChoice : public PopupMessage {
public:
	PopupChoice(const string& MSG="", int W=500, int TH=48, int BH=32);
	virtual ~PopupChoice();
	virtual PopupMessage* Clone() const;

	virtual vector<Object*> getObjects();
	SDL_Rect OkButton() const;

protected:
	Label kButton;
};

class PopupText : public PopupChoice {
public:
	PopupText(const string& MSG="", const string& LIN="", int W=500, int TH=48, int LH=42, int BH=32);
	virtual ~PopupText();
	virtual PopupText* Clone() const;

	virtual vector<Object*> getObjects();
	LineEdit* LEdit();

protected:
	// using LineEdit and Label to simulate an independent text editing object
	LineEdit lineEdit;
	Label lineObject;
};
