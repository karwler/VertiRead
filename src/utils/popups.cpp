#include "engine/world.h"

// POPUP

Popup::Popup(const vec2i& SIZ, float TO) :
	Object(World::winSys()->Resolution()/2, (World::winSys()->Resolution()-SIZ)/2, SIZ, FIX_SIZ),
	timeout(TO)
{}
Popup::~Popup() {}

Popup* Popup::Clone() const {
	return new Popup(*this);
}

void Popup::Tick() {
	if (timeout > 0.f)
		if ((timeout -= World::engine->deltaSeconds()) <= 0.f)
			World::scene()->SetPopup(nullptr);
}

vector<Object*> Popup::getObjects() {
	return {};
}

// POPUP MESSAGE

PopupMessage::PopupMessage(const string& MSG, int W, int TH, int BH, float TO) :
	Popup(vec2i(W, TH+BH), TO),
	title(Object(Anchor(), Pos(), vec2i(W, TH), FIX_SIZ), MSG),
	cButton(Object(Anchor(), Pos()+vec2i(0, TH), vec2i(W, BH), FIX_SIZ), "Ok")
{}
PopupMessage::~PopupMessage() {}

PopupMessage* PopupMessage::Clone() const {
	return new PopupMessage(*this);
}

vector<Object*> PopupMessage::getObjects() {
	return { const_cast<Label*>(&title), const_cast<Label*>(&cButton) };
}

SDL_Rect PopupMessage::CancelButton() const {
	return cButton.getRect();
}

// POPUP CHOICE

PopupChoice::PopupChoice(const string& MSG, int W, int TH, int BH) :
	PopupMessage(MSG, W, TH, BH, 0.f),
	kButton(Object(Anchor(), Pos()+vec2i(W/2, TH), vec2i(W/2, BH), FIX_SIZ), "Ok")
{
	cButton.Size(vec2i(cButton.Size().x/2, cButton.Size().y));
	cButton.text = "Cancel";
}
PopupChoice::~PopupChoice() {}

PopupMessage* PopupChoice::Clone() const {
	return new PopupMessage(*this);
}

vector<Object*> PopupChoice::getObjects() {
	vector<Object*> ret = PopupMessage::getObjects();
	ret.push_back(const_cast<Label*>(&kButton));
	return ret;
}

SDL_Rect PopupChoice::OkButton() const {
	return kButton.getRect();
}

// POPUP TEXT

PopupText::PopupText(const string& MSG, const string& LIN, int W, int TH, int LH, int BH) :
	PopupChoice(MSG, W, TH, BH),
	lineEdit(nullptr, "", LIN)
{
	// resize and reposition everything
	Pos(Anchor()-vec2i(W, TH+LH+BH)/2);
	End(Anchor()+vec2i(W, TH+LH+BH)/2);

	title.Pos(Pos());
	title.Size(vec2i(W, TH));

	lineObject = Label(Object(Anchor(), Pos()+vec2i(0, TH), vec2i(W, LH), FIX_SIZ));
	caretObject = Object(Anchor(), 0, 0, FIX_POS | FIX_END, EColor::highlighted);	// pos and size need to be recalculated every redraw anyway

	cButton.Pos(Pos()+vec2i(0, TH+LH));
	cButton.Size(vec2i(W/2, BH));

	kButton.Pos(Pos()+vec2i(W/2, TH+LH));
	kButton.Size(vec2i(W/2, BH));
}
PopupText::~PopupText() {}

PopupText* PopupText::Clone() const {
	return new PopupText(*this);
}

vector<Object*> PopupText::getObjects() {
	vector<Object*> ret = PopupChoice::getObjects();

	lineObject.text = lineEdit.Editor()->Text();
	ret.push_back(&lineObject);

	int offset = Text(lineEdit.Editor()->Text().substr(0, lineEdit.Editor()->CursorPos()), 0, lineObject.Size().y, 8).size().x;
	caretObject.Pos(lineObject.Pos()+vec2i(offset, 0));
	caretObject.End(caretObject.Pos()+vec2i(5, lineObject.Size().y));
	ret.push_back(&caretObject);

	return ret;
}

LineEdit* PopupText::LEdit() {
	return &lineEdit;
}
