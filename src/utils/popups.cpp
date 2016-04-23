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

PopupMessage::PopupMessage(string MSG, int W, int TH, int BH, float TO) :
	Popup(vec2i(W, TH=BH), TO),
	title(new Label(Object(Anchor(), Pos(), vec2i(W, TH)), MSG)),
	cButton(new ButtonText(Object(Anchor(), Pos()+vec2i(0, TH), vec2i(W, BH)), nullptr, "Ok"))
{}
PopupMessage::~PopupMessage() {}

vector<Object*> PopupMessage::getObjects() const {
	return {title, cButton};
}

SDL_Rect PopupMessage::CancelButton() const {
	return cButton->getRect();
}

// POPUP CHOICE

PopupChoice::PopupChoice(string MSG, int W, int TH, int BH) :
	PopupMessage(MSG, W, TH, BH, 0.f),
	kButton(new ButtonText(Object(Anchor(), Pos()+vec2i(W/2, TH), vec2i(W/2, BH)), nullptr, "Ok"))
{
	// fix cancel button values
	cButton->Size(vec2i(cButton->Size().x/2, cButton->Size().y));
	cButton->text = "Cancel";
}
PopupChoice::~PopupChoice() {}

vector<Object*> PopupChoice::getObjects() const {
	vector<Object*> ret = PopupMessage::getObjects();
	ret.push_back(kButton);
	return ret;
}

SDL_Rect PopupChoice::OkButton() const {
	return kButton->getRect();
}

// POPUP TEXT

PopupText::PopupText(string MSG, string LIN, int W, int TH, int LH, int BH) :
	PopupChoice(MSG, W, TH, BH),
	line(new LineEdit(Object(Anchor(), Pos()+vec2i(0, TH), vec2i(W, LH)), LIN))
{
	// fix button positions
	cButton->Pos(Pos()+vec2i(0,   TH+BH));
	kButton->Pos(Pos()+vec2i(W/2, TH+BH));
}
PopupText::~PopupText() {}

vector<Object*> PopupText::getObjects() const {
	vector<Object*> ret = PopupChoice::getObjects();
	ret.push_back(line);
	return ret;
}

LineEdit* PopupText::Line() const {
	return line;
}
