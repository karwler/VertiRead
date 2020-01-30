#include "engine/world.h"

// WIDGET

Widget::Widget(const Size& relSize, Layout* parent, sizet id) :
	parent(parent),
	pcID(id),
	relSize(relSize)
{}

bool Widget::navSelectable() const {
	return false;
}

bool Widget::hasDoubleclick() const {
	return false;
}

void Widget::setParent(Layout* pnt, sizet id) {
	parent = pnt;
	pcID = id;
}

vec2i Widget::position() const {
	return parent->wgtPosition(pcID);
}

vec2i Widget::size() const {
	return parent->wgtSize(pcID);
}

Rect Widget::frame() const {
	return parent->frame();
}

void Widget::onNavSelect(Direction dir) {
	parent->navSelectNext(pcID, dir.vertical() ? center().x : center().y, dir);
}

// PICTURE

Picture::Picture(const Size& relSize, bool showBG, SDL_Texture* tex, int texMargin, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	tex(tex),
	showBG(showBG),
	texMargin(texMargin)
{}

void Picture::drawSelf() const {
	World::drawSys()->drawPicture(this);
}

Color Picture::color() const {
	return Color::normal;
}

Rect Picture::texRect() const {
	Rect rct = rect();
	return Rect(rct.pos() + texMargin, rct.size() - texMargin * 2);
}

// BUTTON

Button::Button(const Size& relSize, PCall leftCall, PCall rightCall, PCall doubleCall, bool showBG, SDL_Texture* tex, int texMargin, Layout* parent, sizet id) :
	Picture(relSize, showBG, tex, texMargin, parent, id),
	lcall(leftCall),
	rcall(rightCall),
	dcall(doubleCall)
{}

void Button::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		parent->selectWidget(pcID);
		World::prun(lcall, this);
	} else if (mBut == SDL_BUTTON_RIGHT) {
		parent->deselectWidget(pcID);
		World::prun(rcall, this);
	}
}

void Button::onDoubleClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(dcall, this);
}

bool Button::navSelectable() const {
	return lcall || rcall || dcall;
}

bool Button::hasDoubleclick() const {
	return dcall;
}

Color Button::color() const {
	if (parent->getSelected().count(const_cast<Button*>(this)))
		return Color::light;
	if (navSelectable() && World::scene()->select == this)
		return Color::select;
	return Color::normal;
}

// CHECK BOX

CheckBox::CheckBox(const Size& relSize, bool on, PCall leftCall, PCall rightCall, PCall doubleCall, bool showBG, SDL_Texture* tex, int texMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showBG, tex, texMargin, parent, id),
	on(on)
{}

void CheckBox::drawSelf() const {
	World::drawSys()->drawCheckBox(this);
}

void CheckBox::onClick(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		toggle();
	Button::onClick(mPos, mBut);
}

Rect CheckBox::boxRect() const {
	vec2i siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& relSize, int value, int minimum, int maximum, PCall leftCall, PCall rightCall, PCall doubleCall, bool showBG, SDL_Texture* tex, int texMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showBG, tex, texMargin, parent, id),
	val(value),
	vmin(minimum),
	vmax(maximum),
	diffSliderMouse(0)
{}

void Slider::drawSelf() const {
	World::drawSys()->drawSlider(this);
}

void Slider::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Slider::onHold(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		if (int sp = sliderPos(); outRange(mPos.x, sp, sp + barSize))	// if mouse outside of slider
			setSlider(mPos.x - barSize / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(vec2i mPos, vec2i) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)	// if dragging slider stop dragging slider
		World::scene()->capture = nullptr;
}

void Slider::setSlider(int xpos) {
	setVal((xpos - position().x - size().y/4) * vmax / sliderLim());
	World::prun(lcall, this);
}

Rect Slider::barRect() const {
	vec2i siz = size();
	int height = siz.y / 2;
	return Rect(position() + siz.y / 4, vec2i(siz.x - height, height));
}

Rect Slider::sliderRect() const {
	vec2i pos = position(), siz = size();
	return Rect(sliderPos(), pos.y, barSize, siz.y);
}

int Slider::sliderLim() const {
	vec2i siz = size();
	return siz.x - siz.y/2 - barSize;
}

// PROGRESS BAR

ProgressBar::ProgressBar(const Size& relSize, int value, int minimum, int maximum, bool showBG, SDL_Texture* tex, int texMargin, Layout* parent, sizet id) :
	Picture(relSize, showBG, tex, texMargin, parent, id),
	val(value),
	vmin(minimum),
	vmax(maximum)
{}

void ProgressBar::drawSelf() const {
	World::drawSys()->drawProgressBar(this);
}

Rect ProgressBar::barRect() const {
	vec2i siz = size();
	int margin = siz.y / barMarginFactor;
	return Rect(position() + margin, vec2i(val * (siz.x - margin * 2) / (vmax - vmin), siz.y - margin * 2));
}

// LABEL

Label::Label(const Size& relSize, string text, PCall leftCall, PCall rightCall, PCall doubleCall, Alignment alignment, SDL_Texture* tex, bool showBG, int textMargin, int texMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showBG, tex, texMargin, parent, id),
	textTex(nullptr),
	text(std::move(text)),
	textMargin(textMargin),
	align(alignment)
{}

Label::~Label() {
	SDL_DestroyTexture(textTex);
}

void Label::drawSelf() const {
	World::drawSys()->drawLabel(this);
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(const string& str) {
	text = str;
	updateTextTex();
}

void Label::setText(string&& str) {
	text = std::move(str);
	updateTextTex();
}

Rect Label::textFrame() const {
	Rect rct = rect();
	int ofs = textIconOffset();
	return Rect(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).intersect(frame());
}

Rect Label::texRect() const {
	Rect rct = rect();
	rct.h -= texMargin * 2;
	vec2i res = texSize(tex);
	return Rect(rct.pos() + texMargin, vec2i(float(rct.h * res.x) / float(res.y), rct.h));
}

vec2i Label::textPos() const {
	switch (vec2i pos = position(); align) {
	case Alignment::left:
		return vec2i(pos.x + textIconOffset() + textMargin, pos.y);
	case Alignment::center: {
		int iofs = textIconOffset();
		return vec2i(pos.x + iofs + (size().x - iofs - texSize(textTex).x)/2, pos.y); }
	case Alignment::right:
		return vec2i(pos.x + size().x - texSize(textTex).x - textMargin, pos.y);
	}
	return 0;	// to get rid of warning
}

int Label::textIconOffset() const {
	if (tex) {
		vec2i res = texSize(tex);
		return int(float(size().y * res.x) / float(res.y));
	}
	return 0;
}

void Label::updateTextTex() {
	SDL_DestroyTexture(textTex);
	textTex = World::drawSys()->renderText(text, size().y);
}

// SWITCH BOX

SwitchBox::SwitchBox(const Size& relSize, const string* opts, sizet ocnt, string curOption, PCall call, Alignment alignment, SDL_Texture* tex, bool showBG, int textMargin, int texMargin, Layout* parent, sizet id) :
	Label(relSize, std::move(curOption), call, call, nullptr, alignment, tex, showBG, textMargin, texMargin, parent, id),
	options(opts, opts + ocnt),
	curOpt(sizet(std::find(options.begin(), options.end(), text) - options.begin()))
{
	if (curOpt >= options.size())
		curOpt = 0;
}

void SwitchBox::onClick(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		shiftOption(true);
	else if (mBut == SDL_BUTTON_RIGHT)
		shiftOption(false);
	Button::onClick(mPos, mBut);
}

void SwitchBox::shiftOption(bool fwd) {
	if (curOpt += btom<sizet>(fwd); curOpt >= options.size())
		curOpt = fwd ? 0 : options.size() - 1;
	setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(const Size& relSize, string line, PCall leftCall, PCall rightCall, PCall doubleCall, TextType type, bool unfocusConfirm, SDL_Texture* tex, bool showBG, int textMargin, int texMargin, Layout* parent, sizet id) :
	Label(relSize, std::move(line), leftCall, rightCall, doubleCall, Alignment::left, tex, showBG, textMargin, texMargin, parent, id),
	unfocusConfirm(unfocusConfirm),
	textType(type),
	textOfs(0),
	cpos(0),
	oldText(text)
{
	cleanText();
}

void LabelEdit::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		SDL_StartTextInput();
		setCPos(uint(text.length()));
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void LabelEdit::onKeypress(const SDL_Keysym& key) {
	switch (key.scancode) {
	case SDL_SCANCODE_LEFT:	// move caret left
		if (kmodAlt(key.mod))	// if holding alt skip word
			setCPos(findWordStart());
		else if (kmodCtrl(key.mod))	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos > 0)	// otherwise go left by one
			setCPos(jumpCharB(cpos));
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (kmodAlt(key.mod))	// if holding alt skip word
			setCPos(findWordEnd());
		else if (kmodCtrl(key.mod))	// if holding ctrl go to end
			setCPos(uint(text.length()));
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(jumpCharF(cpos));
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (kmodAlt(key.mod)) {	// if holding alt delete left word
			uint id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		} else if (kmodCtrl(key.mod)) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTextTex();
			setCPos(0);
		} else if (cpos > 0) {	// otherwise delete left character
			uint id = jumpCharB(cpos);
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		}
		break;
	case SDL_SCANCODE_DELETE:	// delete right character
		if (kmodAlt(key.mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTex();
		} else if (kmodCtrl(key.mod)) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTex();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, jumpCharF(cpos) - cpos);
			updateTextTex();
		}
		break;
	case SDL_SCANCODE_HOME:	// move caret to beginning
		setCPos(0);
		break;
	case SDL_SCANCODE_END:	// move caret to end
		setCPos(uint(text.length()));
		break;
	case SDL_SCANCODE_V:	// paste text
		if (kmodCtrl(key.mod))
			if (char* ctxt = SDL_GetClipboardText()) {
				onText(ctxt);
				SDL_free(ctxt);
			}
		break;
	case SDL_SCANCODE_C:	// copy text
		if (kmodCtrl(key.mod))
			SDL_SetClipboardText(text.c_str());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (kmodCtrl(key.mod)) {
			SDL_SetClipboardText(text.c_str());
			setText("");
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (kmodCtrl(key.mod))
			setText(std::move(oldText));
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		confirm();
		break;
	case SDL_SCANCODE_ESCAPE:
		cancel();
	}
}

void LabelEdit::onText(const char* str) {
	sizet olen = text.length();
	text.insert(cpos, str);
	cleanText();
	updateTextTex();
	setCPos(cpos + uint(text.length() - olen));
}

vec2i LabelEdit::textPos() const {
	vec2i pos = position();
	return vec2i(pos.x + textOfs + textIconOffset() + textMargin, pos.y);
}

void LabelEdit::setText(const string& str) {
	oldText = std::move(text);
	text = str;
	onTextReset();
}

void LabelEdit::setText(string&& str) {
	oldText = std::move(text);
	text = std::move(str);
	onTextReset();
}

void LabelEdit::onTextReset() {
	cleanText();
	updateTextTex();
	setCPos(uint(text.length()));
}

Rect LabelEdit::caretRect() const {
	vec2i ps = position();
	return { caretPos() + ps.x + textIconOffset() + textMargin, ps.y, caretWidth, size().y };
}

void LabelEdit::setCPos(uint cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - texMargin*2; ce > sx)
		textOfs -= ce - sx;
}

int LabelEdit::caretPos() const {
	return World::drawSys()->textLength(text.substr(0, cpos), size().y) + textOfs;
}

void LabelEdit::confirm() {
	textOfs = 0;
	World::scene()->capture = nullptr;
	SDL_StopTextInput();
	World::prun(lcall, this);
}

void LabelEdit::cancel() {
	textOfs = 0;
	text = oldText;
	updateTextTex();

	World::scene()->capture = nullptr;
	SDL_StopTextInput();
}

uint LabelEdit::jumpCharB(uint i) {
	while (--i && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::jumpCharF(uint i) {
	while (++i < text.length() && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::findWordStart() {
	uint i = cpos;
	if (i == text.length())
		i--;
	else if (notSpace(text[i]) && i)
		if (uint n = jumpCharB(i); isSpace(text[n]))	// skip if first letter of word
			i = n;
	for (; isSpace(text[i]) && i; i--);		// skip first spaces
	for (; notSpace(text[i]) && i; i--);	// skip word
	return i ? i + 1 : i;			// correct position if necessary
}

uint LabelEdit::findWordEnd() {
	uint i = cpos;
	for (; isSpace(text[i]) && i < text.length(); i++);		// skip first spaces
	for (; notSpace(text[i]) && i < text.length(); i++);	// skip word
	return i;
}

void LabelEdit::cleanText() {
	switch (textType) {
	case TextType::sInt:
		text.erase(std::remove_if(text.begin() + pdift(text[0] == '-'), text.end(), [](char c) -> bool { return !isdigit(c); }), text.end());
		break;
	case TextType::sIntSpaced:
		cleanSIntSpacedText();
		break;
	case TextType::uInt:
		text.erase(std::remove_if(text.begin(), text.end(), [](char c) -> bool { return !isdigit(c); }), text.end());
		break;
	case TextType::uIntSpaced:
		cleanUIntSpacedText();
		break;
	case TextType::sFloat:
		cleanSFloatText();
		break;
	case TextType::sFloatSpaced:
		cleanSFloatSpacedText();
		break;
	case TextType::uFloat:
		cleanUFloatText();
		break;
	case TextType::uFloatSpaced:
		cleanUFloatSpacedText();
	}
}

void LabelEdit::cleanSIntSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isdigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUIntSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isdigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == '.'  && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				it++;
		} else if (*it == '.' && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == '.'  && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isdigit(*it))
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return !isdigit(c); });
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else if (*it == '.' && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isdigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

// KEY GETTER

KeyGetter::KeyGetter(const Size& relSize, AcceptType type, Binding::Type binding, Alignment alignment, SDL_Texture* tex, bool showBG, int textMargin, int texMargin, Layout* parent, sizet id) :
	Label(relSize, bindingText(binding, type), nullptr, nullptr, nullptr, alignment, tex, showBG, textMargin, texMargin, parent, id),
	acceptType(type),
	bindingType(binding)
{}

void KeyGetter::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		setText(ellipsisStr);
	} else if (mBut == SDL_BUTTON_RIGHT) {
		clearBinding();
		World::scene()->capture = nullptr;
	}
}

void KeyGetter::onKeypress(const SDL_Keysym& key) {
	if (acceptType == AcceptType::keyboard) {
		World::inputSys()->getBinding(bindingType).setKey(key.scancode);
		setText(SDL_GetScancodeName(key.scancode));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onJButton(uint8 jbutton) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJbutton(jbutton);
		setText(prefButton + to_string(jbutton));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onJHat(uint8 jhat, uint8 value) {
	if (acceptType == AcceptType::joystick) {
		if (value != SDL_HAT_UP && value != SDL_HAT_RIGHT && value != SDL_HAT_DOWN && value != SDL_HAT_LEFT) {
			if (value & SDL_HAT_RIGHT)
				value = SDL_HAT_RIGHT;
			else if (value & SDL_HAT_LEFT)
				value = SDL_HAT_LEFT;
		}
		World::inputSys()->getBinding(bindingType).setJhat(jhat, value);
		setText(prefHat + to_string(jhat) + prefSep + hatNames.at(value));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onJAxis(uint8 jaxis, bool positive) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJaxis(jaxis, positive);
		setText(string(prefAxis) + (positive ? prefAxisPos : prefAxisNeg) + to_string(jaxis));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onGButton(SDL_GameControllerButton gbutton) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGbutton(gbutton);
		setText(gbuttonNames[uint8(gbutton)]);
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onGAxis(SDL_GameControllerAxis gaxis, bool positive) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGaxis(gaxis, positive);
		setText(string(1, (positive ? prefAxisPos : prefAxisNeg)) + gaxisNames[uint8(gaxis)]);
	}
	World::scene()->capture = nullptr;
}

bool KeyGetter::navSelectable() const {
	return true;
}

void KeyGetter::clearBinding() {
	switch (acceptType) {
	case AcceptType::keyboard:
		World::inputSys()->getBinding(bindingType).clearAsgKey();
		break;
	case AcceptType::joystick:
		World::inputSys()->getBinding(bindingType).clearAsgJct();
		break;
	case AcceptType::gamepad:
		World::inputSys()->getBinding(bindingType).clearAsgGct();
	}
	setText("");
}

string KeyGetter::bindingText(Binding::Type binding, KeyGetter::AcceptType accept) {
	switch (Binding& bind = World::inputSys()->getBinding(binding); accept) {
	case AcceptType::keyboard:
		if (bind.keyAssigned())
			return SDL_GetScancodeName(bind.getKey());
		break;
	case AcceptType::joystick:
		if (bind.jbuttonAssigned())
			return prefButton + to_string(bind.getJctID());
		else if (bind.jhatAssigned())
			return prefHat + to_string(bind.getJctID()) + prefSep + hatNames.at(bind.getJhatVal());
		else if (bind.jaxisAssigned())
			return string(prefAxis) + (bind.jposAxisAssigned() ? prefAxisPos : prefAxisNeg) + to_string(bind.getJctID());
		break;
	case AcceptType::gamepad:
		if (bind.gbuttonAssigned())
			return gbuttonNames[uint8(bind.getGbutton())];
		else if (bind.gaxisAssigned())
			return string(1, (bind.gposAxisAssigned() ? prefAxisPos : prefAxisNeg)) + gaxisNames[uint8(bind.getGaxis())];
	}
	return "";
}
