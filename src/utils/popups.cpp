#include "engine/world.h"

// POPUP

Popup::Popup(vec2i SIZ, float TO) :
	Object(World::winSys()->Resolution()/2, (World::winSys()->Resolution()-SIZ)/2, SIZ, FIX_SIZ),
	timeout(TO)
{}
Popup::~Popup() {}

void Popup::Tick() {
	if (timeout > 0.f)
		if ((timeout -= World::engine->deltaSeconds()) <= 0.f)
			World::scene()->SetPopup(nullptr);
}

// POPUP MESSAGE

PopupMessage::PopupMessage(string MSG, float TO) :
	Popup(0, TO),
	msg(MSG),
	msgH(48), butH(48)
{
	ReposResize(vec2i(Text(msg, 0, msgH, 8).size().x, msgH+butH));
}
PopupMessage::~PopupMessage() {}

void PopupMessage::ReposResize(vec2i siz) {
	vec2i res = World::winSys()->Resolution();
	pos = vec2f((res.x-siz.x)/2, (res.y-siz.y)/2);
	end = vec2f((res.x+siz.x)/2, (res.y+siz.y)/2);
}

SDL_Rect PopupMessage::getCancelButton(Text* txt) const {
	vec2i pos(Pos().x, End().y-butH);
	if (txt)
		*txt = Text("Ok", pos, butH, 8);
	return {pos.x, pos.y, Size().x, butH};
}

Text PopupMessage::getMessage() const {
	return Text(msg, Pos(), msgH, 8);
}

// POPUP TEXT

PopupText::PopupText(string MSG, string LIN, float TO) :
	PopupMessage(MSG, TO),
	line(LIN),
	lineH(48)
{
	ReposResize(vec2i(Text(msg, 0, msgH, 8).size().x, msgH+lineH+butH));
}
PopupText::~PopupText() {}

SDL_Rect PopupText::getCancelButton(Text* txt) const {
	vec2i pos(Pos().x+Size().x/2, End().y-butH);
	if (txt)
		*txt = Text("Cancel", pos, butH, 8);
	return {pos.x, pos.y, Size().x/2, butH};
}

SDL_Rect PopupText::getOkButton(Text* txt) const {
	vec2i pos(Pos().x, End().y-butH);
	if (txt)
		*txt = Text("Ok", pos, butH, 8);
	return {pos.x, pos.y, Size().x/2, butH};
}

SDL_Rect PopupText::getLineBox() const {
	return {Pos().x, Pos().y+msgH, Size().x,lineH};
}

Text PopupText::getLine(SDL_Rect* crop) const {
	// set crop here
	return Text(line, vec2i(Pos().x, Pos().y+msgH), lineH, 8);
}

void PopupText::Line(string text) {
	line = text;
}