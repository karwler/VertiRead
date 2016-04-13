#pragma once

#include "objects.h"

class Popup : public Object {
public:
	Popup(vec2i SIZ=vec2i(200, 200), float TO=0.f);
	virtual ~Popup();

	void Tick();

protected:
	float timeout;	// 0 means no timeout
};

class PopupMessage : public Popup {
public:
	PopupMessage(string MSG="", float TO=0.f);
	virtual ~PopupMessage();

	void ReposResize(vec2i siz);

	virtual SDL_Rect getCancelButton(Text* txt=nullptr) const;
	Text getMessage() const;

protected:
	const int msgH, butH;
	string msg;
};

class PopupText : public PopupMessage {
public:
	PopupText(string MSG="", string LIN="", float TO=0.f);
	virtual ~PopupText();

	virtual SDL_Rect getCancelButton(Text* txt=nullptr) const;

	SDL_Rect getOkButton(Text* txt=nullptr) const;
	SDL_Rect getLineBox() const;
	Text getLine(SDL_Rect* crop=nullptr) const;
	void Line(string text);

protected:
	const int lineH;
	string line;

};
