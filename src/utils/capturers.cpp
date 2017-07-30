#include "engine/world.h"

// CAPTURER

Capturer::Capturer(ScrollAreaX1* SA, const string& LBL) :
	ListItem(LBL, SA)
{}
Capturer::~Capturer() {}

void Capturer::OnClick(EClick clickType) {
	World::inputSys()->SetCapture(this);
}

ScrollAreaX1* Capturer::Parent() const {
	return static_cast<ListBox*>(parent);
}

// LINE EDIT

LineEdit::LineEdit(ScrollAreaX1* SA, const string& LBL, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Capturer(SA, LBL),
	editor(TXT, TYPE),
	okCall(KCALL),
	cancelCall(CCALL),
	textPos(0)
{}
LineEdit::~LineEdit() {}

void LineEdit::OnClick(EClick clickType) {
	World::inputSys()->SetCapture(this);
	editor.SetCursor(editor.Text().length());
}

void LineEdit::OnKeypress(SDL_Scancode key) {
	bool redraw = true;
	if (key == SDL_SCANCODE_RIGHT) {
		editor.MoveCursor(true);
		CheckCaretRight();
	} else if (key == SDL_SCANCODE_LEFT) {
		editor.MoveCursor(false);
		CheckCaretLeft();
	} else if (key == SDL_SCANCODE_BACKSPACE) {
		editor.Delete(false);
		CheckCaretLeft();
	} else if (key == SDL_SCANCODE_DELETE)
		editor.Delete(true);
	else if (key == SDL_SCANCODE_RETURN)
		Confirm();
	else if (key == SDL_SCANCODE_ESCAPE)
		Cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->SetRedrawNeeded();
}

void LineEdit::OnJButton(uint8 jbutton) {
	bool redraw = true;
	if (jbutton == 3) {
		editor.Delete(false);
		CheckCaretLeft();
	} else if (jbutton == 0)
		editor.Delete(true);
	else if (jbutton == 2)
		Confirm();
	else if (jbutton == 1)
		Cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->SetRedrawNeeded();
}

void LineEdit::OnJHat(uint8 jhat, uint8 value) {
	bool redraw = true;
	if (value & SDL_HAT_RIGHT) {
		editor.MoveCursor(true);
		CheckCaretRight();
	} else if (value & SDL_HAT_LEFT) {
		editor.MoveCursor(false);
		CheckCaretLeft();
	} else
		redraw = false;

	if (redraw)
		World::winSys()->SetRedrawNeeded();
}

void LineEdit::OnJAxis(uint8 jaxis, bool positive) {}

void LineEdit::OnGButton(uint8 gbutton) {
	bool redraw = true;
	if (gbutton == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
		editor.MoveCursor(true);
		CheckCaretRight();
	} else if (gbutton == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
		editor.MoveCursor(false);
		CheckCaretLeft();
	} else if (gbutton == SDL_CONTROLLER_BUTTON_X) {
		editor.Delete(false);
		CheckCaretLeft();
	} else if (gbutton ==  SDL_CONTROLLER_BUTTON_Y)
		editor.Delete(true);
	else if (gbutton == SDL_CONTROLLER_BUTTON_A)
		Confirm();
	else if (gbutton == SDL_CONTROLLER_BUTTON_B)
		Cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->SetRedrawNeeded();
}

void LineEdit::OnGAxis(uint8 gaxis, bool positive) {}

void LineEdit::OnText(const char* text) {
	editor.Add(text);
	CheckCaretRight();
}

void LineEdit::Confirm() {
	ResetTextPos();
	if (okCall)
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

TextEdit& LineEdit::Editor() {
	return editor;
}

const TextEdit& LineEdit::Editor() const {
	return editor;
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

KeyGetter::KeyGetter(ScrollAreaX1* SA, EAcceptType ACT, Shortcut* SHC) :
	Capturer(SA),
	acceptType(ACT),
	shortcut(SHC)
{}
KeyGetter::~KeyGetter() {}

void KeyGetter::OnKeypress(SDL_Scancode key) {
	if (acceptType == EAcceptType::keyboard && shortcut)
		shortcut->Key(key);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnJButton(uint8 jbutton) {
	if (acceptType == EAcceptType::joystick && shortcut)
		shortcut->JButton(jbutton);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnJHat(uint8 jhat, uint8 value) {
	if (acceptType == EAcceptType::joystick && shortcut) {
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
	if (acceptType == EAcceptType::joystick && shortcut)
		shortcut->JAxis(jaxis, positive);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnGButton(uint8 gbutton) {
	if (acceptType == EAcceptType::gamepad && shortcut)
		shortcut->GButton(gbutton);
	World::inputSys()->SetCapture(nullptr);
}

void KeyGetter::OnGAxis(uint8 gaxis, bool positive) {
	if (acceptType == EAcceptType::gamepad && shortcut)
		shortcut->GAxis(gaxis, positive);
	World::inputSys()->SetCapture(nullptr);
}

string KeyGetter::Text() const {
	if (acceptType == EAcceptType::keyboard) {
		if (shortcut->KeyAssigned())
			return SDL_GetScancodeName(shortcut->Key());
	} else if (acceptType == EAcceptType::joystick) {
		if (shortcut->JButtonAssigned())
			return "B " + to_string(shortcut->JctID());
		if (shortcut->JHatAssigned())
			return "H " + to_string(shortcut->JctID()) + " " + jtHatToStr(shortcut->JHatVal());
		if (shortcut->JAxisAssigned())
			return "A " + string((shortcut->JPosAxisAssigned()) ? "+" : "-") + to_string(shortcut->JctID());
	} else if (acceptType == EAcceptType::gamepad) {
		if (shortcut->GButtonAssigned())
			return gpButtonToStr(shortcut->GctID());
		if (shortcut->GAxisAssigned())
			return (shortcut->GPosAxisAssigned() ? "+" : "-") + gpAxisToStr(shortcut->GctID());
	}
	return "-void-";
}
