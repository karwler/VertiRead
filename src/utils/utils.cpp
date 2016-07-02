#include "engine/world.h"

bool strcmpCI(const string& strl, const string& strr) {
	if (strl.length() != strr.length())
		return false;

	for (size_t i=0; i!=strl.length(); i++)
		if (tolower(strl[i]) != tolower(strr[i]))
			return false;
	return true;
}

bool is_num(const string& str) {
	string::const_iterator it = str.begin();
	while (it != str.end() && isdigit(*it))
		it++;
	return !str.empty() && it == str.end();
}

bool findChar(const string& str, char c, size_t* id) {
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] == c) {
			if (id)
				*id = i;
			return true;
		}
	return false;
}

bool findString(const string& str, const string& c, size_t* id) {
	size_t check = 0;
	for (size_t i=0; i!=str.length(); i++) {
		if (str[i] == c[check]) {
			if (++check == c.length()) {
				if (id)
					*id = i-c.length();
				return true;
			}
		}
		else
			check = 0;
	}
	return false;
}

string modifyCase(string str, ETextCase caseChange) {
	switch (caseChange) {
	case ETextCase::first_upper:
		if (!str.empty())
			str[0] = toupper(str[0]);
		break;
	case ETextCase::all_upper:
		transform(str.begin(), str.end(), str.begin(), toupper);
		break;
	case ETextCase::all_lower:
		transform(str.begin(), str.end(), str.begin(), tolower);
	}
	return str;
}

vector<string> getWords(const string& line, bool skipCommas) {
	vector<string> words;
	string word;
	for (size_t i=0; i!=line.length(); i++) {
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

bool splitIniLine(const string& line, string* arg, string* val, string* key, size_t* id) {
	size_t i0;;
	if (!findChar(line, '=', &i0))
		return false;
	
	if (val)
		*val = line.substr(i0 + 1);
	string left = line.substr(0, i0);

	size_t i1 = 0, i2 = 0;
	findChar(left, '[', &i1);
	findChar(left, ']', &i2);
	if (i1 < i2) {
		if (arg)
			*arg = line.substr(0, i1);
		if (key)
			*key = line.substr(i1+1, i2-i1-1);
	}
	else if (arg)
		*arg = left;

	if (id)
		*id = i0;
	return true;
}

fs::path delExt(const fs::path& path) {
	return path.has_extension() ? path.string().substr(0, path.string().size() - path.extension().string().size()) : path;
}

std::istream& readLine(std::istream& ifs, string& str) {
	str.clear();

	std::istream::sentry se(ifs, true);
	std::streambuf* sbf = ifs.rdbuf();

	while (true) {
		int c = sbf->sbumpc();
		switch (c) {
		case '\n':
			return ifs;
		case '\r':
			if (sbf->sgetc() == '\n')
				sbf->sbumpc();
			return ifs;
		case EOF:
			if (str.empty())
				ifs.setstate(std::ios::eofbit);
			return ifs;
		default:
			str += char(c);
		}
	}
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
	cout << "\nSDL version: min=" << to_str(ver.minor) << " max=" << to_string(ver.major) << " patch=" << to_string(ver.patch) << endl;
	cout << "Platform: " << SDL_GetPlatform() << endl;
	cout << "CPU count: " << SDL_GetCPUCount() << " - " << SDL_GetCPUCacheLineSize() << endl;
	cout << "RAM: " << SDL_GetSystemRAM() << "MB" << endl;
	cout << "\nVideo Drivers:" << endl;
	for (int i=0; i!=SDL_GetNumVideoDrivers(); i++)
		cout << SDL_GetVideoDriver(i) << endl;
	cout << "\nRenderers:" << endl;
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++)
		cout << getRendererName(i) << endl;
	cout << "\nAudio Devices:" << endl;
	for (int i=0; i!=SDL_GetNumAudioDevices(0); i++)
		cout << SDL_GetAudioDeviceName(i, 0) << endl;
	cout << "\nAudio Drivers:" << endl;
	for (int i=0; i!=SDL_GetNumAudioDrivers(); i++)
		cout << SDL_GetAudioDriver(i) << endl;
	cout << endl;
}

string getRendererName(int id) {
	SDL_RendererInfo info;
	if (!SDL_GetRenderDriverInfo(id, &info))
		SDL_GetRenderDriverInfo(-1, &info);
	return info.name;
}

vector<string> getAvailibleRenderers(bool trustedOnly) {
	vector<string> renderers;
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++) {
		string name = getRendererName(i);
		if (!trustedOnly || (trustedOnly && strcmpCI(name, "direct3d")) || (trustedOnly && strcmpCI(name, "opengl")) || (trustedOnly && strcmpCI(name, "software")))
			renderers.push_back(name);
	}
	return renderers;
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

map<EColor, vec4b> GetDefaultColors() {
	map<EColor, vec4b> colors = {
		make_pair(EColor::background, GetDefaultColor(EColor::background)),
		make_pair(EColor::rectangle, GetDefaultColor(EColor::rectangle)),
		make_pair(EColor::highlighted, GetDefaultColor(EColor::highlighted)),
		make_pair(EColor::darkened, GetDefaultColor(EColor::darkened)),
		make_pair(EColor::text, GetDefaultColor(EColor::text))
	};
	return colors;
}

vec4b GetDefaultColor(EColor color) {
	switch (color) {
	case EColor::background:
		return vec4b(10, 10, 10, 255);
	case EColor::rectangle:
		return vec4b(90, 90, 90, 255);
	case EColor::highlighted:
		return vec4b(120, 120, 120, 255);
	case EColor::darkened:
		return vec4b(60, 60, 60, 255);
	case EColor::text:
		return vec4b(210, 210, 210, 255);
	}
}
