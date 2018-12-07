#include "engine/world.h"

// WIDGET

Widget::Widget(const Size& relSize, Layout* parent, sizt id) :
	parent(parent),
	pcID(id),
	relSize(relSize)
{}

bool Widget::navSelectable() const {
	return false;
}

void Widget::setParent(Layout* pnt, sizt id) {
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

void Widget::onNavSelect(const Direction& dir) {
	parent->navSelectNext(pcID, dir.vertical() ? center().x : center().y, dir);
}

// BUTTON

Button::Button(const Size& relSize, PCall leftCall, PCall rightCall, PCall doubleCall, SDL_Texture* background, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Widget(relSize, parent, id),
	tex(background),
	margin(backgroundMargin),
	showBG(showBackground),
	lcall(leftCall),
	rcall(rightCall),
	dcall(doubleCall)
{}

void Button::drawSelf() {
	World::drawSys()->drawButton(this);
}

void Button::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		parent->selectWidget(pcID);
		World::prun(lcall, this);
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Button::onDoubleClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(dcall, this);
}

bool Button::navSelectable() const {
	return lcall || rcall || dcall;
}

Color Button::color() {
	if (parent->getSelected().count(this))
		return Color::light;
	if (navSelectable() && World::scene()->select == this)
		return Color::select;
	return Color::normal;
}

Rect Button::texRect() const {
	Rect rct = rect();
	return Rect(rct.pos() + margin, rct.size() - margin * 2);
}

// CHECK BOX

CheckBox::CheckBox(const Size& relSize, bool on, PCall leftCall, PCall rightCall, PCall doubleCall, SDL_Texture* background, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Button(relSize, leftCall, rightCall, doubleCall, background, showBackground, backgroundMargin, parent, id),
	on(on)
{}

void CheckBox::drawSelf() {
	World::drawSys()->drawCheckBox(this);
}

void CheckBox::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		on = !on;
	Button::onClick(mPos, mBut);
}

Rect CheckBox::boxRect() const {
	vec2i siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& relSize, int value, int minimum, int maximum, PCall leftCall, PCall rightCall, PCall doubleCall, SDL_Texture* background, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Button(relSize, leftCall, rightCall, doubleCall, background, showBackground, backgroundMargin, parent, id),
	val(value),
	vmin(minimum),
	vmax(maximum),
	diffSliderMouse(0)
{}

void Slider::drawSelf() {
	World::drawSys()->drawSlider(this);
}

void Slider::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Slider::onHold(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		if (int sp = sliderPos(); outRange(mPos.x, sp, sp + Default::sbarSize))	// if mouse outside of slider
			setSlider(mPos.x - Default::sbarSize / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(const vec2i& mPos, const vec2i&) {
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
	return Rect(sliderPos(), pos.y, Default::sbarSize, siz.y);
}

int Slider::sliderLim() const {
	vec2i siz = size();
	return siz.x - siz.y/2 - Default::sbarSize;
}

// LABEL

Label::Label(const Size& relSize, const string& text, PCall leftCall, PCall rightCall, PCall doubleCall, Alignment alignment, SDL_Texture* background, int textMargin, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Button(relSize, leftCall, rightCall, doubleCall, background, showBackground, backgroundMargin, parent, id),
	align(alignment),
	textTex(nullptr),
	text(text),
	textMargin(textMargin)
{}

Label::~Label() {
	if (textTex)
		SDL_DestroyTexture(textTex);
}

void Label::drawSelf() {
	World::drawSys()->drawLabel(this);
}

void Label::postInit() {
	updateTex();
}

void Label::setText(const string& str) {
	text = str;
	updateTex();
}

Rect Label::textFrame() const {
	Rect rct = rect();
	int ofs = textIconOffset();
	return Rect(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).getOverlap(frame());
}

Rect Label::texRect() const {
	Rect rct = rect();
	rct.h -= margin * 2;
	vec2i res = texSize(tex);
	return Rect(rct.pos() + margin, vec2i(float(rct.h * res.x) / float(res.y), rct.h));
}

vec2i Label::textPos() const {
	vec2i pos = position();
	if (align == Alignment::left)
		return vec2i(pos.x + textIconOffset() + textMargin, pos.y);
	if (align == Alignment::center)
		return vec2i(pos.x + textIconOffset() + (size().x - texSize(textTex).x)/2, pos.y);
	return vec2i(pos.x + size().x - texSize(textTex).x - textMargin, pos.y);	// Alignment::right
}

int Label::textIconOffset() const {
	if (tex) {
		vec2i res = texSize(tex);
		return int(float(size().y * res.x) / float(res.y));
	}
	return 0;
}

void Label::updateTex() {
	if (textTex)
		SDL_DestroyTexture(textTex);
	textTex = World::drawSys()->renderText(text, size().y);
}

// SWITCH BOX

SwitchBox::SwitchBox(const Size& relSize, const vector<string>& options, const string& curOption, PCall call, Alignment alignment, SDL_Texture* background, int textMargin, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Label(relSize, curOption, call, call, nullptr, alignment, background, textMargin, showBackground, backgroundMargin, parent, id),
	options(options),
	curOpt(0)
{
	for (sizt i = 0; i < options.size(); i++)
		if (curOption == options[i]) {
			curOpt = i;
			break;
		}
}

void SwitchBox::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		shiftOption(1);
	else if (mBut == SDL_BUTTON_RIGHT)
		shiftOption(-1);
	Button::onClick(mPos, mBut);
}

void SwitchBox::shiftOption(int ofs) {
	curOpt = (curOpt + sizt(ofs)) % options.size();
	setText(options[curOpt]);
}

// LINE EDITOR

LineEdit::LineEdit(const Size& relSize, const string& text, PCall leftCall, PCall rightCall, PCall doubleCall, TextType type, SDL_Texture* background, int textMargin, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Label(relSize, text, leftCall, rightCall, doubleCall, Alignment::left, background, textMargin, showBackground, backgroundMargin, parent, id),
	textOfs(0),
	textType(type),
	cpos(0),
	oldText(text)
{
	cleanText();
}

void LineEdit::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		SDL_StartTextInput();
		setCPos(text.length());
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void LineEdit::onKeypress(const SDL_Keysym& key) {
	switch (key.scancode) {
	case SDL_SCANCODE_LEFT:	// move caret left
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordStart());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos > 0)	// otherwise go left by one
			setCPos(cpos - 1);
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordEnd());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl go to end
			setCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(cpos + 1);
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (key.mod & KMOD_LALT) {	// if holding alt delete left word
			sizt id = findWordStart();
			text.erase(id, cpos - id);
			updateTex();
			setCPos(id);
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTex();
			setCPos(0);
		} else if (cpos > 0) {	// otherwise delete left character
			text.erase(cpos - 1, 1);
			updateTex();
			setCPos(cpos - 1);
		}
		break;
	case SDL_SCANCODE_DELETE:	// delete right character
		if (key.mod & KMOD_LALT) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTex();
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTex();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, 1);
			updateTex();
		}
		break;
	case SDL_SCANCODE_HOME:	// move caret to beginning
		setCPos(0);
		break;
	case SDL_SCANCODE_END:	// move caret to end
		setCPos(text.length());
		break;
	case SDL_SCANCODE_V:	// paste text
		if (key.mod & KMOD_CTRL)
			onText(SDL_GetClipboardText());
		break;
	case SDL_SCANCODE_C:	// copy text
		if (key.mod & KMOD_CTRL)
			SDL_SetClipboardText(text.c_str());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (key.mod & KMOD_CTRL) {
			SDL_SetClipboardText(text.c_str());
			setText("");
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (key.mod & KMOD_CTRL) {
			string newOldCopy = oldText;
			setText(newOldCopy);
		}
		break;
	case SDL_SCANCODE_RETURN:
		confirm();
		break;
	case SDL_SCANCODE_ESCAPE:
		cancel();
	}
}

void LineEdit::onText(const string& str) {
	sizt olen = text.length();
	text.insert(cpos, str);
	cleanText();
	updateTex();
	setCPos(cpos + (text.length() - olen));
}

vec2i LineEdit::textPos() const {
	vec2i pos = position();
	return vec2i(pos.x + textOfs + textIconOffset() + textMargin, pos.y);
}

void LineEdit::setText(const string& str) {
	string pot = text;	// use copy in case str is oldText
	text = str;
	oldText = pot;

	cleanText();
	updateTex();
	setCPos(text.length());
}

Rect LineEdit::caretRect() const {
	vec2i ps = position();
	return {caretPos() + ps.x + margin, ps.y, Default::caretWidth, size().y};
}

void LineEdit::setCPos(sizt cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + Default::caretWidth, sx = size().x - margin*2; ce > sx)
		textOfs -= ce - sx;
}

int LineEdit::caretPos() const {
	return World::drawSys()->textLength(text.substr(0, cpos), size().y) + textOfs;
}

void LineEdit::confirm() {
	textOfs = 0;
	World::scene()->capture = nullptr;
	SDL_StopTextInput();
	World::prun(lcall, this);
}

void LineEdit::cancel() {
	textOfs = 0;
	text = oldText;
	updateTex();

	World::scene()->capture = nullptr;
	SDL_StopTextInput();
}

sizt LineEdit::findWordStart() {
	sizt i = cpos;
	if (!isSpace(text[i]) && i > 0 && isSpace(text[i-1]))	// skip if first letter of word
		i--;
	for (; isSpace(text[i]) && i > 0; i--);		// skip first spaces
	for (; !isSpace(text[i]) && i > 0; i--);	// skip word
	return i == 0 ? i : i + 1;			// correct position if necessary
}

sizt LineEdit::findWordEnd() {
	sizt i = cpos;
	for (; isSpace(text[i]) && i < text.length(); i++);		// skip first spaces
	for (; !isSpace(text[i]) && i < text.length(); i++);	// skip word
	return i;
}

void LineEdit::cleanText() {
	switch (textType) {
	case TextType::sInteger:
		cleanUIntText(text[0] == '-');
		break;
	case TextType::sIntegerSpaced:
		cleanSIntSpacedText();
		break;
	case TextType::uInteger:
		cleanUIntText();
		break;
	case TextType::uIntegerSpaced:
		cleanUIntSpacedText();
		break;
	case TextType::sFloating:
		cleanUFloatText(text[0] == '-');
		break;
	case TextType::sFloatingSpaced:
		cleanSFloatSpacedText();
		break;
	case TextType::uFloating:
		cleanUFloatText();
		break;
	case TextType::uFloatingSpaced:
		cleanUFloatSpacedText();
	}
}

void LineEdit::cleanSIntSpacedText(sizt i) {
	for (; isSpace(text[i]); i++);
	if (text[i] == '-')
		i++;

	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (isSpace(text[i])) {
			cleanSIntSpacedText(i + 1);
			break;
		} else
			text.erase(i, 1);
	}
}

void LineEdit::cleanUIntText(sizt i) {
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else
			text.erase(i, 1);
	}
}

void LineEdit::cleanUIntSpacedText() {
	sizt i = 0;
	for (; isSpace(text[i]); i++);

	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (isSpace(text[i]))
			while (isSpace(text[++i]));
		else
			text.erase(i, 1);
	}
}

void LineEdit::cleanSFloatSpacedText(sizt i) {
	for (; isSpace(text[i]); i++);
	if (text[i] == '-')
		i++;

	bool foundDot = false;
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == '.' && !foundDot) {
			foundDot = true;
			i++;
		} else if (isSpace(text[i])) {
			cleanSFloatSpacedText(i + 1);
			break;
		} else
			text.erase(i, 1);
	}
}

void LineEdit::cleanUFloatText(sizt i) {
	bool foundDot = false;
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == '.' && !foundDot) {
			foundDot = true;
			i++;
		} else
			text.erase(i, 1);
	}
}

void LineEdit::cleanUFloatSpacedText() {
	sizt i = 0;
	for (; isSpace(text[i]); i++);

	bool foundDot = false;
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == '.' && !foundDot) {
			foundDot = true;
			i++;
		} else if (isSpace(text[i])) {
			while (isSpace(text[++i]));
			foundDot = false;
		} else
			text.erase(i, 1);
	}
}

// KEY GETTER

KeyGetter::KeyGetter(const Size& relSize, AcceptType type, Binding::Type binding, Alignment alignment, SDL_Texture* background, int textMargin, bool showBackground, int backgroundMargin, Layout* parent, sizt id) :
	Label(relSize, "-void-", nullptr, nullptr, nullptr, alignment, background, textMargin, showBackground, backgroundMargin, parent, id),
	acceptType(type),
	bindingType(binding)
{
	if (Binding& bind = World::inputSys()->getBinding(bindingType); acceptType == AcceptType::keyboard && bind.keyAssigned())
		text = SDL_GetScancodeName(bind.getKey());
	else if (acceptType == AcceptType::joystick) {
		if (bind.jbuttonAssigned())
			text = "B " + to_string(bind.getJctID());
		else if (bind.jhatAssigned())
			text = "H " + to_string(bind.getJctID()) + " " + jtHatToStr(bind.getJhatVal());
		else if (bind.jaxisAssigned())
			text = "A " + string(bind.jposAxisAssigned() ? "+" : "-") + to_string(bind.getJctID());
	} else if (acceptType == AcceptType::gamepad) {
		if (bind.gbuttonAssigned())
			text = enumToStr(Default::gbuttonNames, bind.getGbutton());
		else if (bind.gaxisAssigned())
			text = (bind.gposAxisAssigned() ? "+" : "-") + enumToStr(Default::gaxisNames, bind.getGaxis());
	}
}

void KeyGetter::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		setText("...");
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
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
		setText("B " + to_string(jbutton));
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
		setText("H " + to_string(jhat) + " " + jtHatToStr(value));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onJAxis(uint8 jaxis, bool positive) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJaxis(jaxis, positive);
		setText("A " + string(positive ? "+" : "-") + to_string(jaxis));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onGButton(SDL_GameControllerButton gbutton) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGbutton(gbutton);
		setText(enumToStr(Default::gbuttonNames, gbutton));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onGAxis(SDL_GameControllerAxis gaxis, bool positive) {
	if (acceptType == AcceptType::gamepad) {
		World::inputSys()->getBinding(bindingType).setGaxis(gaxis, positive);
		setText((positive ? "+" : "-") + enumToStr(Default::gaxisNames, gaxis));
	}
	World::scene()->capture = nullptr;
}

bool KeyGetter::navSelectable() const {
	return true;
}
