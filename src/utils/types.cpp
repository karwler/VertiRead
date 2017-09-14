#include "engine/world.h"

// TEXTURE

Texture::Texture() :
	tex(nullptr)
{}

Texture::Texture(const string& FILE) :
	tex(nullptr)
{
	load(FILE);
}

void Texture::load(const string& path) {
	if (tex)
		SDL_DestroyTexture(tex);

	file = path;
	tex = IMG_LoadTexture(World::drawSys()->getRenderer(), file.c_str());
	if (tex)
		SDL_QueryTexture(tex, nullptr, nullptr, &res.x, &res.y);
	else {
		cerr << "couldn't load texture " << file << endl << IMG_GetError << endl;
		res = 0;
		file.clear();
	}
}

void Texture::clear() {
	if (tex)
		SDL_DestroyTexture(tex);
	res = 0;
	file.clear();
}

vec2i Texture::getRes() const {
	return res;
}

string Texture::getFile() const {
	return file;
}

// IMAGE

Image::Image(const vec2i& POS, Texture* TEX, const vec2i& SIZ) :
	pos(POS),
	tex(TEX)
{
	size = SIZ.hasNull() ? tex->getRes() : SIZ;
}

SDL_Rect Image::rect() const {
	return {pos.x, pos.y, size.x, size.y};
}

// FONT

bool FontSet::init(const string& FILE) {
	clear();

	TTF_Font* tmp = TTF_OpenFont(FILE.c_str(), 72);
	if (!tmp) {
		cerr << "couldn't open font " << file << endl << TTF_GetError() << endl;
		file.clear();
		return false;
	}
	TTF_CloseFont(tmp);
	file = FILE;
	return true;
}

void FontSet::clear() {
	for (const pair<int, TTF_Font*>& it : fonts)
		TTF_CloseFont(it.second);
	fonts.clear();
}

bool FontSet::canRun() const {
	return !file.empty();
}

TTF_Font* FontSet::getFont(int size) {
	return (canRun() && fonts.count(size) == 0) ? addSize(size) : fonts.at(size);
}

TTF_Font* FontSet::addSize(int size) {
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.insert(make_pair(size, font));
	else
		cerr << "couldn't load font " << file << endl << TTF_GetError() << endl;
	return font;
}

vec2i FontSet::textSize(const string& text, int size) {
	vec2i siz;
	TTF_Font* font = getFont(size);
	if (font)
		TTF_SizeUTF8(font, text.c_str(), &siz.x, &siz.y);
	return siz;
}

// TEXT

Text::Text(const string& TXT, const vec2i& POS, int HEI, EColor CLR) :
	pos(POS),
	height(float(HEI) * Default::textHeightScale),
	color(CLR),
	text(TXT)
{}

void Text::setPosToRect(const SDL_Rect& rect, ETextAlign align) {
	int len = size().x;

	if (align == ETextAlign::left)
		pos.x = rect.x + Default::textOffset;
	else if (align == ETextAlign::center)
		pos.x = rect.x + (rect.w-len)/2;
	else if (align == ETextAlign::right)
		pos.x = rect.x + rect.w - len - Default::textOffset;
	pos.y = rect.y;
}

vec2i Text::size() const {
	return World::library()->getFonts().textSize(text, height);
}

// TEXT EDIT

TextEdit::TextEdit(const string& TXT, ETextType TYPE, size_t CPOS) :
	type(TYPE),
	text(TXT)
{
	setCaretPos(CPOS);
	checkText();
}

size_t TextEdit::getCaretPos() const {
	return cpos;
}

void TextEdit::setCaretPos(size_t pos) {
	cpos = (pos > text.length()) ? text.length() : pos;
}

void TextEdit::moveCaret(bool right, bool loop) {
	if (loop) {
		if (right)
			cpos = (cpos == text.length()) ? 0 : cpos+1;
		else
			cpos = (cpos == 0) ? text.length() : cpos-1;
	} else {
		if (right && cpos != text.length())
			cpos++;
		else if (!right && cpos != 0)
			cpos--;
	}
}

string TextEdit::getText() const {
	return text;
}

void TextEdit::setText(const string& str, bool resetCpos) {
	text = str;
	
	if (resetCpos)
		cpos = 0;
	else
		checkCaret();
}

void TextEdit::addText(const string& str) {
	text.insert(cpos, str);
	cpos += str.length();
	checkText();
}

void TextEdit::delChar(bool current) {
	if (current) {
		if (cpos != text.length())
			text.erase(cpos, 1);
	} else if (cpos != 0) {
		cpos--;
		text.erase(cpos, 1);
	}
}

void TextEdit::checkCaret() {
	if (cpos > text.length())
		cpos = text.length();
}

void TextEdit::checkText() {
	if (type == ETextType::integer)
		cleanIntString(text);
	else if (type == ETextType::floating)
		cleanFloatString(text);
	else
		return;
	checkCaret();
}

void TextEdit::cleanIntString(string& str) {
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] < '0' || str[i] > '9') {
			str.erase(i, 1);
			i--;
		}
}

void TextEdit::cleanFloatString(string& str) {
	bool foundDot = false;
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] < '0' || str[i] > '9') {
			if (str[i] == '.' && !foundDot)
				foundDot = true;
			else {
				str.erase(i, 1);
				i--;
			}
		}
}

// CLICK TYPE

ClickType::ClickType(uint8 BTN, uint8 NUM) :
	button(BTN),
	clicks(NUM)
{}

// CLICK STAMP

ClickStamp::ClickStamp(Widget* wgt, uint8 BUT, const vec2i& POS) :
	widget(wgt),
	button(BUT),
	mPos(POS)
{}

void ClickStamp::reset() {
	widget = nullptr;
	button = 0;
}

// CONTROLLER

Controller::Controller() :
	joystick(nullptr),
	gamepad(nullptr)
{}

bool Controller::open(int id) {
	if (SDL_IsGameController(id))
		gamepad = SDL_GameControllerOpen(id);
	
	joystick = (gamepad) ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id);
	return joystick;
}

void Controller::close() {
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = nullptr;
		joystick = nullptr;
	} else if (joystick) {
		SDL_JoystickClose(joystick);
		joystick = nullptr;
	}
}

// SHORTCUT

Shortcut::Shortcut() :
	asg(ASG_NONE)
{}

Shortcut::~Shortcut() {}

SDL_Scancode Shortcut::getKey() const {
	return key;
}

bool Shortcut::keyAssigned() const {
	return asg & ASG_KEY;
}

void Shortcut::clearAsgKey() {
	asg &= ~ASG_KEY;
}

void Shortcut::setKey(SDL_Scancode KEY) {
	key = KEY;
	asg |= ASG_KEY;
}

uint8 Shortcut::getJctID() const {
	return jctID;
}

bool Shortcut::jctAssigned() const {
	return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

void Shortcut::clearAsgJct() {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

bool Shortcut::jbuttonAssigned() const {
	return asg & ASG_JBUTTON;
}

void Shortcut::setJbutton(uint8 BUT) {
	jctID = BUT;

	clearAsgJct();
	asg |= ASG_JBUTTON;
}

bool Shortcut::jaxisAssigned() const {
	return asg & (ASG_JAXIS_P | ASG_JAXIS_N);
}

bool Shortcut::jposAxisAssigned() const {
	return asg & ASG_JAXIS_P;
}

bool Shortcut::jnegAxisAssigned() const {
	return asg & ASG_JAXIS_N;
}

void Shortcut::setJaxis(uint8 AXIS, bool positive) {
	jctID = AXIS;

	clearAsgJct();
	asg |= (positive) ? ASG_JAXIS_P : ASG_JAXIS_N;
}

uint8 Shortcut::getJhatVal() const {
	return jHatVal;
}

bool Shortcut::jhatAssigned() const {
	return asg & ASG_JHAT;
}

void Shortcut::setJhat(uint8 HAT, uint8 VAL) {
	jctID = HAT;
	jHatVal = VAL;

	clearAsgJct();
	asg |= ASG_JHAT;
}

uint8 Shortcut::getGctID() const {
	return gctID;
}

bool Shortcut::gctAssigned() const {
	return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

void Shortcut::clearAsgGct() {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

bool Shortcut::gbuttonAssigned() const {
	return asg & ASG_GBUTTON;
}

void Shortcut::gbutton(uint8 BUT) {
	gctID = BUT;

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

bool Shortcut::gaxisAssigned() const {
	return asg & (ASG_GAXIS_P | ASG_GAXIS_N);
}

bool Shortcut::gposAxisAssigned() const {
	return asg & ASG_GAXIS_P;
}

bool Shortcut::gnegAxisAssigned() const {
	return asg & ASG_GAXIS_N;
}

void Shortcut::setGaxis(uint8 AXIS, bool positive) {
	gctID = AXIS;

	clearAsgGct();
	asg |= (positive) ? ASG_GAXIS_P : ASG_GAXIS_N;
}

Shortcut::EAssgnment operator~(Shortcut::EAssgnment a) {
	return static_cast<Shortcut::EAssgnment>(~static_cast<uint8>(a));
}
Shortcut::EAssgnment operator&(Shortcut::EAssgnment a, Shortcut::EAssgnment b) {
	return static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
Shortcut::EAssgnment operator&=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b) {
	return a = static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
Shortcut::EAssgnment operator^(Shortcut::EAssgnment a, Shortcut::EAssgnment b) {
	return static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
Shortcut::EAssgnment operator^=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b) {
	return a = static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
Shortcut::EAssgnment operator|(Shortcut::EAssgnment a, Shortcut::EAssgnment b) {
	return static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) | static_cast<uint8>(b));
}
Shortcut::EAssgnment operator|=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b) {
	return a = static_cast<Shortcut::EAssgnment>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// SHORTCUT KEY

ShortcutKey::ShortcutKey(void (Program::*CALL)()) :
	Shortcut(),
	call(CALL)
{}
ShortcutKey::~ShortcutKey() {}

// SHORTCUT AXIS

ShortcutAxis::ShortcutAxis(void (Program::*CALL)(float)) :
	Shortcut(),
	call(CALL)
{}
ShortcutAxis::~ShortcutAxis() {}

// PLAYLIST

Playlist::Playlist(const string& NAME, const vector<string>& SGS, const vector<string>& BKS) :
	name(NAME),
	songs(SGS),
	books(BKS)
{}

string Playlist::songPath(size_t id) const {
	return isAbsolute(songs[id]) ? songs[id] : parentPath(World::library()->getSettings().playlistPath() + name) + songs[id];
}

Directory::Directory(const string& NAME, const vector<string>& DIRS, const vector<string>& FILS) :
	name(NAME),
	dirs(DIRS),
	files(FILS)
{}

// INI LINE

IniLine::IniLine() :
	type(Type::av)
{}

IniLine::IniLine(const string& ARG, const string& VAL) :
	type(Type::av),
	arg(ARG),
	val(VAL)
{}

IniLine::IniLine(const string& ARG, const string& KEY, const string& VAL) :
	type(Type::akv),
	arg(ARG),
	key(KEY),
	val(VAL)
{}

IniLine::IniLine(const string& TIT) :
	type(Type::title),
	arg(TIT)
{}

string IniLine::line() const {
	if (type == Type::av)
		return arg + '=' + val;
	if (type == Type::akv)
		return arg + '[' + key + "]=" + val;
	return '[' + arg + ']';
}

void IniLine::setVal(const string& ARG, const string& VAL) {
	type = Type::av;
	arg = ARG;
	key.clear();
	val = VAL;
}

void IniLine::setVal(const std::string& ARG, const std::string& KEY, const std::string& VAL) {
	type = Type::akv;
	arg = ARG;
	key = KEY;
	val = VAL;
}

void IniLine::setTitle(const string& TIT) {
	type == Type::title;
	arg = TIT;
	key.clear();
	val.clear();
}

bool IniLine::setLine(const string& lin) {
	// check if title
	if (lin[0] == '[' && lin[lin.length()-1] == ']') {
		arg = lin.substr(1, lin.length()-2);
		type == Type::title;
		return true;
	}

	// find position of the = to split line into argument and value
	size_t i0;;
	if (!findChar(lin, '=', i0))
		return false;

	val = lin.substr(i0 + 1);
	string left = lin.substr(0, i0);

	// get key if availible
	size_t i1 = 0, i2 = 0;
	type = (findChar(left, '[', i1) && findChar(left, ']', i2)) ? Type::akv : Type::av;
	if (type == Type::akv && i1 < i2) {
		arg = lin.substr(0, i1);
		key = lin.substr(i1+1, i2-i1-1);
	} else
		arg = left;
	return true;
}

void IniLine::clear() {
	type = Type::av;
	arg.clear();
	val.clear();
	key.clear();
}

// EXCEPTION

Exception::Exception(const string& MSG, int RV) :
	message(MSG),
	retval(RV)
{}

void Exception::printMessage() {
	cerr << "ERROR: " << message << " (code " << retval << ")" << endl;
}
