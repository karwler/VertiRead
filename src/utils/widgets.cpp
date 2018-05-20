#include "engine/world.h"

// SIZE

Size::Size(int PIX) :
	usePix(true),
	pix(PIX)
{}

Size::Size(float PRC) :
	usePix(false),
	prc(PRC)
{}

void Size::set(int PIX) {
	usePix = true;
	pix = PIX;
}

void Size::set(float PRC) {
	usePix = false;
	prc = PRC;
}

// WIDGET

Widget::Widget(const Size& SIZ) :
	parent(nullptr),
	pcID(SIZE_MAX),
	relSize(SIZ)
{}

void Widget::setParent(Layout* PNT, sizt ID) {
	parent = PNT;
	pcID = ID;
}

vec2i Widget::position() const {
	return parent->wgtPosition(pcID);
}

vec2i Widget::size() const {
	return parent->wgtSize(pcID);
}

vec2i Widget::center() const {
	return position() + size() / 2;
}

SDL_Rect Widget::rect() const {
	vec2i pos = position();
	vec2i siz = size();
	return {pos.x, pos.y, siz.x, siz.y};
}

SDL_Rect Widget::parentFrame() const {
	return parent->frame();
}

void Widget::onSelectUp() {
	parent->selectNext(pcID, center().x, 0);
}

void Widget::onSelectDown() {
	parent->selectNext(pcID, center().x, 1);
}

void Widget::onSelectLeft() {
	parent->selectNext(pcID, center().y, 2);
}

void Widget::onSelectRight() {
	parent->selectNext(pcID, center().y, 3);
}

// BUTTON

Button::Button(const Size& SIZ, void (Program::*LCL)(Button*), void (Program::*RCL)(Button*), void (Program::*DCL)(Button*)) :
	Widget(SIZ),
	lcall(LCL),
	rcall(RCL),
	dcall(DCL)
{}

void Button::drawSelf() {
	World::drawSys()->drawButton(this);
}

void Button::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && lcall)
		(World::program()->*lcall)(this);
	else if (mBut == SDL_BUTTON_RIGHT && rcall)
		(World::program()->*rcall)(this);
}

void Button::onDoubleClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && dcall)
		(World::program()->*dcall)(this);
}

// CHECK BOX

CheckBox::CheckBox(const Button& BASE, bool ON) :
	Button(BASE),
	on(ON)
{}

void CheckBox::drawSelf() {
	World::drawSys()->drawCheckBox(this);
}

void CheckBox::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		on = !on;
	Button::onClick(mPos, mBut);
}

SDL_Rect CheckBox::boxRect() const {
	vec2i pos = position();
	vec2i siz = size();
	int margin = (siz.x > siz.y) ? siz.y/4 : siz.x/4;
	return {pos.x+margin, pos.y+margin, siz.x-margin*2, siz.y-margin*2};
}

// SLIDER

Slider::Slider(const Button& BASE, int VAL, int MIN, int MAX) :
	Button(BASE),
	val(VAL),
	min(MIN),
	max(MAX)
{}

void Slider::drawSelf() {
	World::drawSys()->drawSlider(this);
}

void Slider::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT && rcall)
		(World::program()->*rcall)(this);
}

void Slider::onHold(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;

		int sp = sliderPos();
		if (outRange(mPos.x, sp, sp + Default::sliderWidth))	// if mouse outside of slider
			setSlider(mPos.x - Default::sliderWidth/2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(const vec2i& mPos, const vec2i& mMov) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)	// if dragging slider stop dragging slider
		World::scene()->capture = nullptr;
}

void Slider::setSlider(int xpos) {
	setVal((xpos - position().x - size().y/4) * max / sliderLim());
	if (lcall)
		(World::program()->*lcall)(this);
}

void Slider::setVal(int VAL) {
	val = bringIn(VAL, min, max);
}

SDL_Rect Slider::barRect() const {
	vec2i pos = position();
	vec2i siz = size();
	int margin = siz.y / 4;
	int height = siz.y / 2;
	return {pos.x + margin, pos.y + margin, siz.x - height, height};
}

SDL_Rect Slider::sliderRect() const {
	vec2i pos = position();
	vec2i siz = size();
	return {sliderPos(), pos.y, Default::sliderWidth, siz.y};
}

int Slider::sliderPos() const {
	return position().x + size().y/4 + val * sliderLim() / max;
}

int Slider::sliderLim() const {
	vec2i siz = size();
	return siz.x - siz.y/2 - Default::sliderWidth;
}

// PICTURE

Picture::Picture(const Button& BASE, const string& TEX) :
	Button(BASE)
{
	tex = World::drawSys()->loadTexture(TEX, res);
	if (tex)
		file = TEX;
}

Picture::~Picture() {
	if (tex)
		SDL_DestroyTexture(tex);
}

void Picture::drawSelf() {
	World::drawSys()->drawPicture(this);
}

// LABEL

Label::Label(const Button& BASE, const string& TXT, Alignment ALG) :
	Button(BASE),
	align(ALG),
	text(TXT),
	tex(nullptr)
{}

Label::~Label() {
	if (tex)
		SDL_DestroyTexture(tex);
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

vec2i Label::textPos() const {
	vec2i pos = position();
	if (align == Alignment::left)
		return vec2i(pos.x + Default::textOffset, pos.y);
	if (align == Alignment::center)
		return vec2i(pos.x + (size().x - textSize.x)/2, pos.y);
	return vec2i(pos.x + size().x - textSize.x - Default::textOffset, pos.y);	// Alignment::right
}

SDL_Rect Label::textRect() const {
	vec2i pos = textPos();
	return {pos.x, pos.y, textSize.x, textSize.y};
}

void Label::updateTex() {
	if (tex)
		SDL_DestroyTexture(tex);
	tex = World::drawSys()->renderText(text, size().y, textSize);
}

// SWITCH BOX

SwitchBox::SwitchBox(const Button& BASE, const vector<string>& OPTS, const string& COP, Alignment ALG) :
	Label(BASE, COP, ALG),
	options(OPTS)
{
	for (sizt i=0; i<options.size(); i++)
		if (COP == options[i]) {
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
	curOpt = (curOpt + ofs) % options.size();
	setText(options[curOpt]);
}

// LINE EDITOR

LineEdit::LineEdit(const Button& BASE, const string& TXT, TextType TYP) :
	Label(BASE, TXT, Alignment::left),
	textType(TYP),
	textOfs(0),
	cpos(0),
	oldText(TXT)
{
	cleanText();
}

void LineEdit::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		SDL_StartTextInput();
		setCPos(text.length());
	} else if (mBut == SDL_BUTTON_RIGHT && rcall)
		(World::program()->*rcall)(this);
}

void LineEdit::onKeypress(const SDL_Keysym& key) {
	if (key.scancode == SDL_SCANCODE_LEFT) {	// move caret left
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordStart());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos != 0)	// otherwise go left by one
			setCPos(cpos - 1);
	} else if (key.scancode == SDL_SCANCODE_RIGHT) {	// move caret right
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordEnd());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl go to end
			setCPos(text.length());
		else if (cpos != text.length())	// otherwise go right by one
			setCPos(cpos + 1);
	} else if (key.scancode == SDL_SCANCODE_BACKSPACE) {	// delete left
		if (key.mod & KMOD_LALT) {	// if holding alt delete left word
			sizt id = findWordStart();
			text.erase(id, cpos - id);
			updateTex();
			setCPos(id);
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTex();
			setCPos(0);
		} else if (cpos != 0) {	// otherwise delete left character
			text.erase(cpos - 1);
			updateTex();
			setCPos(cpos - 1);
		}
	} else if (key.scancode == SDL_SCANCODE_DELETE) {	// delete right character
		if (key.mod & KMOD_LALT) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTex();
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTex();
		} else if (cpos != text.length()) {	// otherwise delete right character
			text.erase(cpos, 1);
			updateTex();
		}
	} else if (key.scancode == SDL_SCANCODE_HOME)	// move caret to beginning
		setCPos(0);
	else if (key.scancode == SDL_SCANCODE_END)	// move caret to end
		setCPos(text.length());
	else if (key.scancode == SDL_SCANCODE_V) {	// paste text
		if (key.mod & KMOD_CTRL)
			onText(SDL_GetClipboardText());
	} else if (key.scancode == SDL_SCANCODE_C) {	// copy text
		if (key.mod & KMOD_CTRL)
			SDL_SetClipboardText(text.c_str());
	} else if (key.scancode == SDL_SCANCODE_X) {	// cut text
		if (key.mod & KMOD_CTRL) {
			SDL_SetClipboardText(text.c_str());
			setText("");
		}
	} else if (key.scancode == SDL_SCANCODE_Z || key.scancode == SDL_SCANCODE_Y) {	// set text to old text
		if (key.mod & KMOD_CTRL) {
			string newOldCopy = oldText;
			setText(newOldCopy);
		}
	} else if (key.scancode == SDL_SCANCODE_RETURN)
		confirm();
	else if (key.scancode == SDL_SCANCODE_ESCAPE)
		cancel();
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
	return vec2i(pos.x + textOfs + Default::textOffset, pos.y);
}

void LineEdit::setText(const string& str) {
	oldText = text;
	text = str;
	cleanText();
	updateTex();
	setCPos(text.length());
}

SDL_Rect LineEdit::caretRect() const {
	vec2i ps = position();
	return {caretPos() + ps.x + Default::textOffset, ps.y, Default::caretWidth, size().y};
}

void LineEdit::setCPos(sizt cp) {
	cpos = cp;
	int cl = caretPos();
	int ce = cl + Default::caretWidth;
	int sx = size().x - Default::textOffset*2;

	if (cl < 0)
		textOfs -= cl;
	else if (ce > sx)
		textOfs -= ce - sx;
}

int LineEdit::caretPos() const {
	return World::drawSys()->textLength(text.substr(0, cpos), size().y) + textOfs;
}

void LineEdit::confirm() {
	textOfs = 0;
	World::scene()->capture = nullptr;
	SDL_StopTextInput();

	if (lcall)
		(World::program()->*lcall)(this);
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
	if (text[i] != ' ' && i > 0 && text[i-1] == ' ')	// skip if first letter of word
		i--;
	while (text[i] == ' ' && i > 0)	// skip first spaces
		i--;
	while (text[i] != ' ' && i > 0)	// skip word
		i--;
	return (i == 0) ? i : i+1;	// correct position if necessary
}

sizt LineEdit::findWordEnd() {
	sizt i = cpos;
	while (text[i] == ' ' && i < text.length())	// skip first spaces
		i++;
	while (text[i] != ' ' && i < text.length())	// skip word
		i++;
	return i;
}

void LineEdit::cleanText() {
	if (textType == TextType::sInteger)
		cleanUIntText((text[0] == '-') ? 1 : 0);
	else if (textType == TextType::sIntegerSpaced)
		cleanSIntSpacedText();
	else if (textType == TextType::uInteger)
		cleanUIntText();
	else if (textType == TextType::uIntegerSpaced)
		cleanUIntSpacedText();
	else if (textType == TextType::sFloating)
		cleanUFloatText((text[0] == '-') ? 1 : 0);
	else if (textType == TextType::sFloatingSpaced)
		cleanSFloatSpacedText();
	else if (textType == TextType::uFloating)
		cleanUFloatText();
	else if (textType == TextType::uFloatingSpaced)
		cleanUFloatSpacedText();
}

void LineEdit::cleanSIntSpacedText(sizt i) {
	while (text[i] == ' ')
		i++;
	if (text[i] == '-')
		i++;

	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == ' ') {
			cleanSIntSpacedText(i+1);
			break;
		} else
			text.erase(i);
	}
}

void LineEdit::cleanUIntText(sizt i) {
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else
			text.erase(i);
	}
}

void LineEdit::cleanUIntSpacedText() {
	sizt i = 0;
	while (text[i] == ' ')
		i++;

	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == ' ')
			while (text[++i] == ' ');
		else
			text.erase(i);
	}
}

void LineEdit::cleanSFloatSpacedText(sizt i) {
	while (text[i] == ' ')
		i++;
	if (text[i] == '-')
		i++;

	bool foundDot = false;
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == '.' && !foundDot) {
			foundDot = true;
			i++;
		} else if (text[i] == ' ') {
			cleanSFloatSpacedText(i+1);
			break;
		} else
			text.erase(i);
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
			text.erase(i);
	}
}

void LineEdit::cleanUFloatSpacedText() {
	sizt i = 0;
	while (text[i] == ' ')
		i++;

	bool foundDot = false;
	while (i < text.length()) {
		if (isDigit(text[i]))
			i++;
		else if (text[i] == '.' && !foundDot) {
			foundDot = true;
			i++;
		} else if (text[i] == ' ') {
			while (text[++i] == ' ');
			foundDot = false;
		} else
			text.erase(i);
	}
}

// KEY GETTER

KeyGetter::KeyGetter(const Button& BASE, AcceptType ACT, Binding::Type BND) :
	Label(BASE, "-void-", Alignment::center),
	acceptType(ACT),
	bindingType(BND)
{
	Binding& binding = World::inputSys()->getBinding(bindingType);
	if (acceptType == AcceptType::keyboard && binding.keyAssigned())
		text = SDL_GetScancodeName(binding.getKey());
	else if (acceptType == AcceptType::joystick) {
		if (binding.jbuttonAssigned())
			text = "B " + ntos(binding.getJctID());
		else if (binding.jhatAssigned())
			text = "H " + ntos(binding.getJctID()) + " " + jtHatToStr(binding.getJhatVal());
		else if (binding.jaxisAssigned())
			text = "A " + string(binding.jposAxisAssigned() ? "+" : "-") + ntos(binding.getJctID());
	} else if (acceptType == AcceptType::gamepad) {
		if (binding.gbuttonAssigned())
			text = enumToStr(Default::gbuttonNames, binding.getGbutton());
		else if (binding.gaxisAssigned())
			text = (binding.gposAxisAssigned() ? "+" : "-") + enumToStr(Default::gaxisNames, binding.getGaxis());
	}
}

void KeyGetter::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		setText("...");
	} else if (mBut == SDL_BUTTON_RIGHT && rcall)
		(World::program()->*rcall)(this);
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
		setText("B " + ntos(jbutton));
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
		setText("H " + ntos(jhat) + " " + jtHatToStr(value));
	}
	World::scene()->capture = nullptr;
}

void KeyGetter::onJAxis(uint8 jaxis, bool positive) {
	if (acceptType == AcceptType::joystick) {
		World::inputSys()->getBinding(bindingType).setJaxis(jaxis, positive);
		setText("A " + string(positive ? "+" : "-") + ntos(jaxis));
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
