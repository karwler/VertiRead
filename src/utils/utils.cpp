#include "engine/world.h"

bool isNumber(string str) {
	string::const_iterator it = str.begin();
	while (it != str.end() && isdigit(*it))
		it++;
	return !str.empty() && it == str.end();
}

int findChar(string str, char c) {
	for (uint i = 0; i != str.size(); i++)
		if (str[i] == c)
			return i;
	return -1;
}

vector<string> getWords(string line, bool skipCommas) {
	vector<string> words;
	string word;
	for (uint i = 0; i <= line.length(); i++) {
		if (line[i] == ' ' || (!skipCommas && line[i] == ',') || i == line.length()) {
			if (word.length() != 0)
				words.push_back(word);
			word.clear();
		}
		else
			word += line[i];
	}
	return words;
}

int splitIniLine(string line, string* arg, string* val, string* key) {
	int i0 = findChar(line, '=');
	if (i0 == -1)
		return -1;
	if (val)
		*val = line.substr(i0 + 1);
	string left = line.substr(0, i0);
	int i1 = findChar(left, '[');
	int i2 = findChar(left, ']');
	if (i1 < i2 && i1 != -1) {
		if (key) *key = line.substr(i1 + 1, i2 - i1 - 1);
		if (arg) *arg = line.substr(0, i1);
	}
	else if (arg)
		*arg = left;
	return i0;
}

fs::path removeExtension(fs::path path) {
	return (path.has_extension()) ? path.string().substr(0, path.string().size() - path.extension().string().size()) : path;
}

bool inRect(const SDL_Rect& rect, vec2i point) {
	return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}

bool needsCrop(const SDL_Rect& crop) {
	return crop.x != 0 || crop.y != 0 || crop.w != 0 || crop.h != 0;
}

SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame) {
	item.w += item.x;
	item.h += item.y;
	frame.w += frame.x;
	frame.h += frame.y;

	if (item.h < frame.y || item.y > frame.h || item.w < frame.x || item.x > frame.w)	// if outside of frame
		return {0, 0, item.w-item.x, item.h-item.y};

	SDL_Rect crop = { 0, 0, 0, 0 };
	if (item.x < frame.x && item.w > frame.x)	// left
		crop.x = frame.x - item.x;
	if (item.x < frame.w && item.w > frame.w)	// right
		crop.w = item.w - frame.w;
	if (item.y < frame.y && item.h > frame.y)	// top
		crop.y = frame.y - item.y;
	if (item.y < frame.h && item.h > frame.h)	// bottom
		crop.h = item.h - frame.h;

	return crop;
}

SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop) {
	vec2i temp(rect.w, rect.h);
	rect = { rect.x + crop.x, rect.y + crop.y, rect.w - crop.x - crop.w, rect.h - crop.y - crop.h };
	crop = { crop.x, crop.y, temp.x - crop.x - crop.w, temp.y - crop.y - crop.h };

	SDL_Surface* sheet = SDL_CreateRGBSurface(surface->flags, crop.w, crop.h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_BlitSurface(surface, &crop, sheet, 0);
	return sheet;
}

vec2i textureSize(string path) {
	SDL_Surface* surf = IMG_Load(path.c_str());
	if (!surf)
		return vec2i();
	vec2i size(surf->w, surf->h);
	SDL_FreeSurface(surf);
	return size;
}

void PrintInfo() {
	SDL_version ver;
	SDL_GetVersion(&ver);
	cout << "\nSDL version:" << int(ver.minor) << " - " << int(ver.major) << " p" << int(ver.patch) << endl;
	cout << "Platform: " << SDL_GetPlatform() << endl;
	cout << "CPU count: " << SDL_GetCPUCount() << " - " << SDL_GetCPUCacheLineSize() << endl;
	cout << "RAM: " << SDL_GetSystemRAM() << "MB" << endl;
	cout << "\nVideo Drivers:" << endl;
	for (int i = 0; i != SDL_GetNumVideoDrivers(); i++)
		cout << SDL_GetVideoDriver(i) << endl;
	cout << "\nRenderers:" << endl;
	for (int i = 0; i != SDL_GetNumRenderDrivers(); i++)
		cout << getRendererName(i) << endl;
	cout << "\nAudio Devices:" << endl;
	for (int i = 0; i != SDL_GetNumAudioDevices(0); i++)
		cout << SDL_GetAudioDeviceName(i, 0) << endl;
	cout << "\nAudio Drivers:" << endl;
	for (int i = 0; i != SDL_GetNumAudioDrivers(); i++)
		cout << SDL_GetAudioDriver(i) << endl;
	cout << endl;
}

string getRendererName(int id) {
	SDL_RendererInfo info;
	if (!SDL_GetRenderDriverInfo(id, &info))
		SDL_GetRenderDriverInfo(-1, &info);
	return info.name;
}

string wtos(wstring wstr) {
	string sstr;
	for (wchar_t& c : wstr)
		sstr += c;
	return sstr;
}

bool stob(string str) {
	return str == "true";
}

string btos(bool b) {
	return b ? "true" : "false";
}

SDL_Keysym stok(string str) {
	SDL_Keysym key;
	key.scancode = SDL_GetScancodeFromName(str.c_str());
	key.sym = SDL_GetKeyFromScancode(key.scancode);
	return key;
}

string ktos(SDL_Keysym key) {
	return SDL_GetScancodeName(key.scancode);
}

vec2i pix(vec2f p) {
	return vec2i(p.x * World::winSys()->Resolution().x, p.y * World::winSys()->Resolution().y);
}

int pixX(float p) {
	return p * World::winSys()->Resolution().x;
}

int pixY(float p) {
	return p * World::winSys()->Resolution().y;
}

vec2f prc(vec2i p) {
	return vec2f(float(p.x) / float(World::winSys()->Resolution().x), float(p.y) / float(World::winSys()->Resolution().y));
}

float prcX(int p) {
	return float(p) / float(World::winSys()->Resolution().x);
}

float prcY(int p) {
	return float(p) / float(World::winSys()->Resolution().y);
}
