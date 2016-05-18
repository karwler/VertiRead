#include "engine/world.h"

// CAPTURER

Capturer::Capturer(const Object& BASE) :
	Object(BASE)
{}
Capturer::~Capturer() {}

Capturer* Capturer::Clone() const {
	return new Capturer(*this);
}

void Capturer::OnClick() {
	World::inputSys()->SetCapture(this);
}

void Capturer::OnKeypress(SDL_Scancode key) {
	// ms visual is being a bitch
}

// LINE EDIT

LineEdit::LineEdit(const Object& BASE, const string& LBL, const string& TXT, void (Program::*KCALL)(TextEdit*), void (Program::*CCALL)(), ETextType TYPE) :
	Capturer(BASE),
	label(LBL),
	type(TYPE),
	editor(CheckText(TXT)),
	textPos(0)
{
	okCall = KCALL ? KCALL : &Program::Event_TextCaptureOk;
	cancelCall = CCALL ? CCALL : &Program::Event_Back;
}
LineEdit::~LineEdit() {}

LineEdit* LineEdit::Clone() const {
	return new LineEdit(*this);
}

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

void LineEdit::AddText(const string& text) {
	editor.Add(CheckText(text));
}

Text LineEdit::getText(vec2i* sideCrop) const {
	int offset = getLabel().size().x;
	if (sideCrop) {
		*sideCrop = vec2i(textPos, textPos + Text(editor.getText(), 0, Size().y, 8).size().x - offset - End().x);
		if (sideCrop->y < 0)
			sideCrop->y = 0;
	}
	return Text(editor.getText(), Pos()+vec2i(offset-textPos, 0), Size().y, 8);
}

Text LineEdit::getLabel() const {
	return Text(label, Pos(), Size().y, 8);
}

TextEdit* LineEdit::Editor() const {
	return const_cast<TextEdit*>(&editor);
}

string LineEdit::CheckText(string str) {
	bool foundDot = false;
	for (uint i=0; i!=str.length(); i++) {
		if (type == ETextType::integer) {
			if (str[i] < '0' || str[i] > '9')
				str.erase(i, 1);
		}
		else if (type == ETextType::floating) {
			if (str[i] == '.') {
				if (foundDot)
					str.erase(i, 1);
				else
					foundDot = true;
			}
			else if (str[i] < '0' || str[i] > '9')
				str.erase(i, 1);
		}
	}
	return str;
}

// KEY GETTER

KeyGetter::KeyGetter(const Object& BASE, const string& LBL, SDL_Scancode KEY, void (Program::*CALLB)(SDL_Scancode)) :
	Capturer(BASE),
	label(LBL),
	key(KEY)
{
	callback = CALLB ? CALLB : &Program::Event_KeyCaptureOk;
}
KeyGetter::~KeyGetter() {}

KeyGetter* KeyGetter::Clone() const {
	return new KeyGetter(*this);
}

void KeyGetter::OnClick() {
	Capturer::OnClick();
	key = SDL_SCANCODE_ESCAPE;
}

void KeyGetter::OnKeypress(SDL_Scancode KEY) {
	key = KEY;
	(World::program()->*callback)(key);
}

Text KeyGetter::getText() const {
	return Text(SDL_GetScancodeName(key), Pos()+vec2i(getLabel().size().x, 0), Size().y, 8);
}

Text KeyGetter::getLabel() const {
	return Text(label, Pos(), Size().y, 8);
}
