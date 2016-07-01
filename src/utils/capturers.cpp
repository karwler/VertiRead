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
	editor(TXT),
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
	case SDL_SCANCODE_LEFT:
		editor.MoveCursor(false);
		CheckCaretLeft();
		break;
	case SDL_SCANCODE_RIGHT:
		editor.MoveCursor(true);
		CheckCaretRight();
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
		World::engine->SetRedrawNeeded();
}

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
