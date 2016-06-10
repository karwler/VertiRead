#include "engine/world.h"

bool is_num(const string& str) {
	string::const_iterator it = str.begin();
	while (it != str.end() && isdigit(*it))
		it++;
	return !str.empty() && it == str.end();
}

int findChar(const string& str, char c) {
	for (int i=0; i!=str.length(); i++)
		if (str[i] == c)
			return i;
	return -1;
}

int findString(const string& str, const string& c) {
	int check = 0;
	for (int i=0; i!=str.length(); i++) {
		if (str[i] == c[check]) {
			if (++check == c.length())
				return i-c.length();
		}
		else
			check = 0;
	}
	return -1;
}

vector<string> getWords(const string& line, bool skipCommas) {
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

int splitIniLine(const string& line, string* arg, string* val, string* key) {
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

fs::path removeExtension(const fs::path& path) {
	return (path.has_extension()) ? path.string().substr(0, path.string().size() - path.extension().string().size()) : path;
}

bool inRect(const SDL_Rect& rect, vec2i point) {
	return rect.w != 0 && rect.h != 0 && point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}

bool needsCrop(const SDL_Rect& crop) {
	return crop.x != 0 || crop.y != 0 || crop.w != 0 || crop.h != 0;
}

SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame) {
	item.w += item.x;
	item.h += item.y;
	frame.w += frame.x;
	frame.h += frame.y;

	vec2i outRet(item.w-item.x, item.h-item.y);	// return values in case that the item is outta frame
	SDL_Rect crop = { 0, 0, 0, 0 };

	if (item.w < frame.x)	// left
		crop.w = -outRet.x;
	else if (item.x < frame.x)
		crop.x = frame.x - item.x;

	if (item.x > frame.w)	// right
		crop.w = outRet.x;
	else if (item.w > frame.w)
		crop.w = item.w - frame.w;

	if (item.h < frame.y)	// top
		crop.h = -outRet.y;
	else if (item.y < frame.y)
		crop.y = frame.y - item.y;

	if (item.y > frame.h)	// bottom
		crop.h = outRet.y;
	else if (item.h > frame.h)
		crop.h = item.h - frame.h;

	return crop;
}

SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop) {
	return { rect.x+crop.x, rect.y+crop.y, rect.w-crop.x-crop.w, rect.h-crop.y-crop.h };
}

SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop) {
	vec2i temp(rect.w, rect.h);
	rect = cropRect(rect, crop);
	crop = { crop.x, crop.y, temp.x - crop.x - crop.w, temp.y - crop.y - crop.h };

	SDL_Surface* sheet = SDL_CreateRGBSurface(surface->flags, crop.w, crop.h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_BlitSurface(surface, &crop, sheet, 0);
	return sheet;
}

void PrintInfo() {
	SDL_version ver;
	SDL_GetVersion(&ver);
	cout << "\nSDL version: " << int(ver.minor) << " - " << int(ver.major) << " p" << int(ver.patch) << endl;
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

bool stob(const string& str) {
	return str == "true";
}

string btos(bool b) {
	return b ? "true" : "false";
}

vec2i pix(const vec2f& p) {
	vec2i res = World::winSys()->Resolution();
	return vec2i(p.x * res.x, p.y *res.y);
}

int pixX(float p) {
	return p * World::winSys()->Resolution().x;
}

int pixY(float p) {
	return p * World::winSys()->Resolution().y;
}

vec2f prc(const vec2i& p) {
	vec2i res = World::winSys()->Resolution();
	return vec2f(float(p.x) / float(res.x), float(p.y) / float(res.y));
}

float prcX(int p) {
	return float(p) / float(World::winSys()->Resolution().x);
}

float prcY(int p) {
	return float(p) / float(World::winSys()->Resolution().y);
}
