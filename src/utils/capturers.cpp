#include "engine/world.h"

// CAPTURER

Capturer::Capturer(ScrollAreaItems* SA, const string& LBL) :
	ListItem(LBL, SA)
{}
Capturer::~Capturer() {}

void Capturer::onClick(ClickType click) {
	World::inputSys()->setCaptured(this);
}

// LINE EDIT

LineEdit::LineEdit(ScrollAreaItems* SA, const string& LBL, const string& TXT, ETextType TYPE, void (Program::*KCALL)(const string&), void (Program::*CCALL)()) :
	Capturer(SA, LBL),
	editor(TXT, TYPE),
	okCall(KCALL),
	cancelCall(CCALL),
	textPos(0)
{}
LineEdit::~LineEdit() {}

void LineEdit::onClick(ClickType click) {
	World::inputSys()->setCaptured(this);
	editor.setCaretPos(editor.getText().length());
}

void LineEdit::onKeypress(SDL_Scancode key) {
	bool redraw = true;
	if (key == SDL_SCANCODE_RIGHT) {
		editor.moveCaret(true);
		checkCaretRight();
	} else if (key == SDL_SCANCODE_LEFT) {
		editor.moveCaret(false);
		checkCaretLeft();
	} else if (key == SDL_SCANCODE_BACKSPACE) {
		editor.delChar(false);
		checkCaretLeft();
	} else if (key == SDL_SCANCODE_DELETE)
		editor.delChar(true);
	else if (key == SDL_SCANCODE_RETURN)
		confirm();
	else if (key == SDL_SCANCODE_ESCAPE)
		cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->setRedrawNeeded();
}

void LineEdit::onJButton(uint8 jbutton) {
	bool redraw = true;
	if (jbutton == 3) {
		editor.delChar(false);
		checkCaretLeft();
	} else if (jbutton == 0)
		editor.delChar(true);
	else if (jbutton == 2)
		confirm();
	else if (jbutton == 1)
		cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->setRedrawNeeded();
}

void LineEdit::onJHat(uint8 jhat, uint8 value) {
	bool redraw = true;
	if (value & SDL_HAT_RIGHT) {
		editor.moveCaret(true);
		checkCaretRight();
	} else if (value & SDL_HAT_LEFT) {
		editor.moveCaret(false);
		checkCaretLeft();
	} else
		redraw = false;

	if (redraw)
		World::winSys()->setRedrawNeeded();
}

void LineEdit::onJAxis(uint8 jaxis, bool positive) {}

void LineEdit::onGButton(uint8 gbutton) {
	bool redraw = true;
	if (gbutton == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
		editor.moveCaret(true);
		checkCaretRight();
	} else if (gbutton == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
		editor.moveCaret(false);
		checkCaretLeft();
	} else if (gbutton == SDL_CONTROLLER_BUTTON_X) {
		editor.delChar(false);
		checkCaretLeft();
	} else if (gbutton ==  SDL_CONTROLLER_BUTTON_Y)
		editor.delChar(true);
	else if (gbutton == SDL_CONTROLLER_BUTTON_A)
		confirm();
	else if (gbutton == SDL_CONTROLLER_BUTTON_B)
		cancel();
	else
		redraw = false;

	if (redraw)
		World::winSys()->setRedrawNeeded();
}

void LineEdit::onGAxis(uint8 gaxis, bool positive) {}

void LineEdit::onText(const char* text) {
	editor.addText(text);
	checkCaretRight();
}

void LineEdit::confirm() {
	resetTextPos();
	if (okCall)
		(World::program()->*okCall)(editor.getText());
	World::inputSys()->setCaptured(nullptr);
}

void LineEdit::cancel() {
	resetTextPos();
	if (cancelCall)
		(World::program()->*cancelCall)();
	World::inputSys()->setCaptured(nullptr);
}

int LineEdit::getTextPos() const {
	return textPos;
}

void LineEdit::resetTextPos() {
	textPos = 0;
}

TextEdit& LineEdit::getEditor() {
	return editor;
}

Text LineEdit::text() const {
	ListBox* box = static_cast<ListBox*>(parent);

	int offset = Text(label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	return Text(editor.getText(), vec2i(offset-textPos, 0), Default::itemHeight);
}

SDL_Rect LineEdit::caretRect() const {
	ListBox* box = static_cast<ListBox*>(parent);

	int offset = Text(editor.getText().substr(0, editor.getCaretPos()), 0, Default::itemHeight).size().x - textPos;
	return {offset, 0, Default::caretWidth, Default::itemHeight};
}

void LineEdit::checkCaretRight() {
	if (ListBox* box = dynamic_cast<ListBox*>(parent)) {
		SDL_Rect caret = caretRect();
		int diff = caret.x + caret.w - parent->size().x + parent->barW() + Text(label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
		if (diff > 0)
			textPos += diff;
	}
}

void LineEdit::checkCaretLeft() {
	if (parent) {
		SDL_Rect caret = caretRect();
		if (caret.x < 0)
			textPos += caret.x;
	}
}

// KEY GETTER

KeyGetter::KeyGetter(ScrollAreaItems* SA, EAcceptType ACT, Shortcut* SHC) :
	Capturer(SA),
	acceptType(ACT),
	shortcut(SHC)
{}
KeyGetter::~KeyGetter() {}

void KeyGetter::onKeypress(SDL_Scancode key) {
	if (acceptType == EAcceptType::keyboard && shortcut)
		shortcut->setKey(key);
	World::inputSys()->setCaptured(nullptr);
}

void KeyGetter::onJButton(uint8 jbutton) {
	if (acceptType == EAcceptType::joystick && shortcut)
		shortcut->setJbutton(jbutton);
	World::inputSys()->setCaptured(nullptr);
}

void KeyGetter::onJHat(uint8 jhat, uint8 value) {
	if (acceptType == EAcceptType::joystick && shortcut) {
		if (value != SDL_HAT_UP && value != SDL_HAT_RIGHT && value != SDL_HAT_DOWN && value != SDL_HAT_LEFT) {
			if (value & SDL_HAT_RIGHT)
				value = SDL_HAT_RIGHT;
			else if (value & SDL_HAT_LEFT)
				value = SDL_HAT_LEFT;
		}
		shortcut->setJhat(jhat, value);
	}
	World::inputSys()->setCaptured(nullptr);
}

void KeyGetter::onJAxis(uint8 jaxis, bool positive) {
	if (acceptType == EAcceptType::joystick && shortcut)
		shortcut->setJaxis(jaxis, positive);
	World::inputSys()->setCaptured(nullptr);
}

void KeyGetter::onGButton(uint8 gbutton) {
	if (acceptType == EAcceptType::gamepad && shortcut)
		shortcut->gbutton(gbutton);
	World::inputSys()->setCaptured(nullptr);
}

void KeyGetter::onGAxis(uint8 gaxis, bool positive) {
	if (acceptType == EAcceptType::gamepad && shortcut)
		shortcut->setGaxis(gaxis, positive);
	World::inputSys()->setCaptured(nullptr);
}

string KeyGetter::text() const {
	if (acceptType == EAcceptType::keyboard) {
		if (shortcut->keyAssigned())
			return SDL_GetScancodeName(shortcut->getKey());
	} else if (acceptType == EAcceptType::joystick) {
		if (shortcut->jbuttonAssigned())
			return "B " + to_string(shortcut->getJctID());
		if (shortcut->jhatAssigned())
			return "H " + to_string(shortcut->getJctID()) + " " + jtHatToStr(shortcut->getJhatVal());
		if (shortcut->jaxisAssigned())
			return "A " + string((shortcut->jposAxisAssigned()) ? "+" : "-") + to_string(shortcut->getJctID());
	} else if (acceptType == EAcceptType::gamepad) {
		if (shortcut->gbuttonAssigned())
			return gpButtonToStr(shortcut->getGctID());
		if (shortcut->gaxisAssigned())
			return (shortcut->gposAxisAssigned() ? "+" : "-") + gpAxisToStr(shortcut->getGctID());
	}
	return "-void-";
}
