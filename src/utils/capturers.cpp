#include "engine/world.h"

// CAPTURER

Capturer::Capturer(ListBox* SA, const string& LBL) :
	ListItem(LBL, SA)
{}
Capturer::~Capturer() {}

void Capturer::OnClick(EClick clickType) {
	World::inputSys()->SetCapture(this);
}

void Capturer::OnKeypress(SDL_Scancode key) {
	// ms visual is being a bitch which is the reason why this definition exists
}

ListBox* Capturer::Parent() const {
	return static_cast<ListBox*>(parent);
}

// LINE EDIT

LineEdit::LineEdit(ListBox* SA, const string& LBL, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Capturer(SA, LBL),
	editor(TXT),
	cancelCall(CCALL),
	textPos(0)
{
	okCall = KCALL ? KCALL : &Program::Event_TextCaptureOk;
}
LineEdit::~LineEdit() {}

void LineEdit::OnClick(EClick clickType) {
	Capturer::OnClick(clickType);
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
		Confirm();
		break;
	case SDL_SCANCODE_ESCAPE:
		if (cancelCall)
			(World::program()->*cancelCall)();
		World::inputSys()->SetCapture(nullptr);
	}
}

void LineEdit::Confirm() {
	(World::program()->*okCall)(editor.Text());
	World::inputSys()->SetCapture(nullptr);
}

TextEdit* LineEdit::Editor() {
	return &editor;
}

// KEY GETTER

KeyGetter::KeyGetter(ListBox* SA, const string& LBL, SDL_Scancode* KEY) :
	Capturer(SA, LBL),
	key(KEY)
{}
KeyGetter::~KeyGetter() {}

void KeyGetter::OnKeypress(SDL_Scancode KEY) {
	if (key)
		*key = KEY;
	World::inputSys()->SetCapture(nullptr);
}

string KeyGetter::KeyName() const {
	return (key) ? SDL_GetScancodeName(*key) : "";
}
