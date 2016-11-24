#include "engine/world.h"

// CAPTURER

Capturer::Capturer(ListBox* SA, const string& LBL) :
	ListItem(LBL, SA)
{}
Capturer::~Capturer() {}

void Capturer::OnClick(EClick clickType) {
	World::inputSys()->SetCapture(this);
}

ListBox* Capturer::Parent() const {
	return static_cast<ListBox*>(parent);
}

// LINE EDIT

LineEdit::LineEdit(ListBox* SA, const string& LBL, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Capturer(SA, LBL),
	editor(TXT, TYPE),
	cancelCall(CCALL),
	textPos(0)
{
	okCall = KCALL ? KCALL : &Program::Event_TextCaptureOk;
}
LineEdit::~LineEdit() {}

void LineEdit::OnClick(EClick clickType) {
	World::inputSys()->SetCapture(this);
	editor.SetCursor(0);
}

void LineEdit::OnKeypress(SDL_Scancode key) {
	bool redraw = true;
	switch (key) {
	case SDL_SCANCODE_RIGHT:
		editor.MoveCursor(true);
		CheckCaretRight();
		break;
	case SDL_SCANCODE_LEFT:
		editor.MoveCursor(false);
		CheckCaretLeft();
		break;
	case SDL_SCANCODE_BACKSPACE:
		editor.Delete(false);
		CheckCaretLeft();
		break;
	case SDL_SCANCODE_DELETE:
		editor.Delete(true);
		break;
	case SDL_SCANCODE_RETURN:
		Confirm();
		break;
	case SDL_SCANCODE_ESCAPE:
		Cancel();
		break;
	default:
		redraw = false;
	}
	if (redraw)
		World::engine()->SetRedrawNeeded();
}

void LineEdit::OnJButton(uint8 jbutton) {
	bool redraw = true;
	switch (jbutton) {
	case 3:
		editor.Delete(false);
		CheckCaretLeft();
		break;
	case 0:
		editor.Delete(true);
		break;
	case 2:
		Confirm();
		break;
	case 1:
		Cancel();
		break;
	default:
		redraw = false;
	}
	if (redraw)
		World::engine()->SetRedrawNeeded();
}

void LineEdit::OnJHat(uint8 jhat, uint8 value) {
	bool redraw = true;
	if (value & SDL_HAT_RIGHT) {
		editor.MoveCursor(true);
		CheckCaretRight();
	}
	else if (value & SDL_HAT_LEFT) {
		editor.MoveCursor(false);
		CheckCaretLeft();
	}
	else
		redraw = false;
	if (redraw)
		World::engine()->SetRedrawNeeded();
}

void LineEdit::OnJAxis(uint8 jaxis, bool positive) {}

void LineEdit::OnGButton(uint8 gbutton) {
	bool redraw = true;
	switch (gbutton) {
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		editor.MoveCursor(true);
		CheckCaretRight();
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		editor.MoveCursor(false);
		CheckCaretLeft();
		break;
	case SDL_CONTROLLER_BUTTON_X:
		editor.Delete(false);
		CheckCaretLeft();
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		editor.Delete(true);
		break;
	case SDL_CONTROLLER_BUTTON_A:
		Confirm();
		break;
	case SDL_CONTROLLER_BUTTON_B:
		Cancel();
		break;
	default:
		redraw = false;
	}
	if (redraw)
		World::engine()->SetRedrawNeeded();
}

void LineEdit::OnGAxis(uint8 gaxis, bool positive) {}

void LineEdit::Confirm() {
	ResetTextPos();
	(World::program()->*okCall)(editor.Text());
	World::inputSys()->SetCapture(nullptr);
}

void LineEdit::Cancel() {
	ResetTextPos();
	if (cancelCall)
		(World::program()->*cancelCall)();
	World::inputSys()->SetCapture(nullptr);
}

int LineEdit::TextPos() const {
	return textPos;
}

void LineEdit::ResetTextPos() {
	textPos = 0;
}

TextEdit* LineEdit::Editor() {
	return &editor;
}

Text LineEdit::getText() const {
	ListBox* box = static_cast<ListBox*>(parent);

	int offset = Text(label, 0, box->ItemH()).size().x + 20;
	return Text(editor.Text(), vec2i(offset-textPos, 0), box->ItemH());
}

SDL_Rect LineEdit::getCaret() const {
	ListBox* box = static_cast<ListBox*>(parent);

	int offset = Text(editor.Text().substr(0, editor.CursorPos()), 0, box->ItemH()).size().x - textPos;
	return { offset, 0, 5, box->ItemH() };
}

void LineEdit::CheckCaretRight() {
	if (ListBox* box = dynamic_cast<ListBox*>(parent)) {
		SDL_Rect caret = getCaret();
		int diff = caret.x + caret.w - parent->Size().x + parent->BarW() + Text(label, 0, box->ItemH()).size().x + 20;
		if (diff > 0)
			textPos += diff;
	}
}

void LineEdit::CheckCaretLeft() {
	if (parent) {
		SDL_Rect caret = getCaret();
		if (caret.x < 0)
			textPos += caret.x;
	}
}

// KEY GETTER

KeyGetter::KeyGetter(ListBox* SA, const string& LBL, Shortcut* SHC) :
	Capturer(SA, LBL),
	shortcut(SHC)
{}
KeyGetter::~KeyGetter() {}

void KeyGetter::OnKeypress(SDL_Scancode key) {
	if (shortcut)
		shortcut->Key(key);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnJButton(uint8 jbutton) {
	if (shortcut)
		shortcut->JButton(jbutton);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnJHat(uint8 jhat, uint8 value) {
	if (shortcut) {
		if (value != SDL_HAT_UP && value != SDL_HAT_RIGHT && value != SDL_HAT_DOWN && value != SDL_HAT_LEFT) {
			if (value & SDL_HAT_RIGHT)
				value = SDL_HAT_RIGHT;
			else if (value & SDL_HAT_LEFT)
				value = SDL_HAT_LEFT;
		}
		shortcut->JHat(jhat, value);
	}
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnJAxis(uint8 jaxis, bool positive) {
	if (shortcut)
		shortcut->JAxis(jaxis, positive);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnGButton(uint8 gbutton) {
	if (shortcut)
		shortcut->GButton(gbutton);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnGAxis(uint8 gaxis, bool positive) {
	if (shortcut)
		shortcut->GAxis(gaxis, positive);
	World::inputSys()->SetCapture(nullptr);
}

string KeyGetter::Text() const {
	string line = string((shortcut->KeyAssigned()) ? SDL_GetScancodeName(shortcut->Key()) : "-void-") +  "  | ";

	if (shortcut->JButtonAssigned())
		line += "B " + to_string(shortcut->JctID());
	else if (shortcut->JHatAssigned())
		line += "H " + to_string(shortcut->JctID()) + " " + jtHatToStr(shortcut->JHatVal());
	else if (shortcut->JAxisAssigned())
		line += "A " + string((shortcut->JPosAxisAssigned()) ? "+" : "-") + to_string(shortcut->JctID());
	else
		line += " -void-";
	line += "  | ";

	if (shortcut->GButtonAssigned())
		line += gpButtonToStr(shortcut->GctID());
	else if (shortcut->GAxisAssigned())
		line += ((shortcut->GPosAxisAssigned()) ? "+" : "-") + gpAxisToStr(shortcut->GctID());
	else
		line += " -void-";
	return line;
}
