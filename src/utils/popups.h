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

class PopupChoice : public PopupMessage {
public:
	PopupChoice(string MSG="");
	virtual ~PopupChoice();

	virtual SDL_Rect getCancelButton(Text* txt=nullptr) const;
	SDL_Rect getOkButton(Text* txt=nullptr) const;
};

class PopupText : public PopupChoice {
public:
	PopupText(string MSG="", string LIN="");
	virtual ~PopupText();

	SDL_Rect getLineBox() const;
	Text getLine(SDL_Rect* crop=nullptr) const;
	TextEdit* Line();
	void Line(string text);

protected:
	const int lineH;
	TextEdit line;
};
