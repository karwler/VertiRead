#include "engine/world.h"

// CAPTURER

Capturer::Capturer(ListBox* SA, const string& LBL) :
	ListItem(LBL, SA)
{}
Capturer::~Capturer() {}

void Capturer::OnClick() {
	World::inputSys()->SetCapture(this);
}

void Capturer::OnKeypress(SDL_Scancode key) {
	// ms visual is being a bitch
}

ListBox* Capturer::Parent() const {
	return static_cast<ListBox*>(parent);
}

// LINE EDIT

LineEdit::LineEdit(ListBox* SA, const string& LBL, const string& TXT, void (Program::*KCALL)(TextEdit*), void (Program::*CCALL)(), ETextType TYPE) :
	Capturer(SA, LBL),
	editor(TXT),
	textPos(0)
{
	okCall = KCALL ? KCALL : &Program::Event_TextCaptureOk;
	cancelCall = CCALL ? CCALL : &Program::Event_Back;
}
LineEdit::~LineEdit() {}

void LineEdit::OnClick() {
	Capturer::OnClick();
	editor.SetCursor(0);
}

void LineEdit::OnKeypress(SDL_Scancode key) {
	switch (key) {
	case SDL_SCANCODE_LEFT:
		editor.MoveCursor(-1);
		break;
	case SDL_SCANCODE_RIGHT:
		editor.MoveCursor(1);
		break;
	case SDL_SCANCODE_BACKSPACE:
		editor.Delete(false);
		break;
	case SDL_SCANCODE_DELETE:
		editor.Delete(true);
		break;
	case SDL_SCANCODE_RETURN:
		(World::program()->*okCall)(&editor);
		break;
	case SDL_SCANCODE_ESCAPE:
		(World::program()->*cancelCall)();
	}
}

TextEdit* LineEdit::Editor() {
	return &editor;
}

// KEY GETTER

KeyGetter::KeyGetter(ListBox* SA, const string& LBL, SDL_Scancode KEY, void (Program::*CALLB)(SDL_Scancode)) :
	Capturer(SA, LBL),
	key(KEY)
{
	callback = CALLB ? CALLB : &Program::Event_KeyCaptureOk;
}
KeyGetter::~KeyGetter() {}

void KeyGetter::OnClick() {
	Capturer::OnClick();
	key = SDL_SCANCODE_ESCAPE;
}

void KeyGetter::OnKeypress(SDL_Scancode KEY) {
	key = KEY;
	(World::program()->*callback)(key);
}

string KeyGetter::KeyName() const {
	return SDL_GetScancodeName(key);
}
