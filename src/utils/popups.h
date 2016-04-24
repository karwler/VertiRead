#pragma once

#include "capturers.h"

class Popup : public Object {
public:
	Popup(vec2i SIZ=vec2i(500, 200), float TO=0.f);
	virtual ~Popup();

	void Tick();
	virtual vector<Object*> getObjects() const;

protected:
	float timeout;	// 0 means no timeout
};

class PopupMessage : public Popup {
public:
	PopupMessage(string MSG="", int W=500, int TH=48, int BH=32, float TO=0.f);
	virtual ~PopupMessage();

	virtual vector<Object*> getObjects() const;
	SDL_Rect CancelButton() const;

protected:
	kptr<Label> title;
	kptr<ButtonText> cButton;
};

class PopupChoice : public PopupMessage {
public:
	PopupChoice(string MSG="", int W=500, int TH=48, int BH=32);
	virtual ~PopupChoice();

	virtual vector<Object*> getObjects() const;
	SDL_Rect OkButton() const;

protected:
	kptr<ButtonText> kButton;
};

class PopupText : public PopupChoice {
public:
	PopupText(string MSG="", string LIN="", int W=500, int TH=48, int LH=42, int BH=32);
	virtual ~PopupText();

	virtual vector<Object*> getObjects() const;
	LineEdit* Line() const;

protected:
	kptr<LineEdit> line;
};
