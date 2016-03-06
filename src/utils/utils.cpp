#include "utils.h"

bool isNumber(string str) {
	string::const_iterator it = str.begin();
	while (it != str.end() && isdigit(*it))
		it++;
	return !str.empty() && it == str.end();
}

int findChar(string str, char c) {
	for (int i = 0; i != str.size(); i++)
		if (str[i] == c)
			return i;
	return -1;
}

vector<string> getWords(string line) {
	vector<string> words;
	string word;
	for (uint i = 0; i <= line.length(); i++) {
		if (line[i] == ' ' || line[i] == ',' || i == line.length()) {
			if (word.length() != 0)
				words.push_back(word);
			word.clear();
		}
		else
			word += line[i];
	}
	return words;
}

fs::path removeExtension(fs::path path) {
	return (path.has_extension()) ? path.string().substr(0, path.string().size() - path.extension().string().size()) : path;
}

bool inRect(SDL_Rect rect, vec2i point) {
	return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}

vector<string> getAvailableImageExts() {
	vector<string> exts(4);
	exts[0] = ".jpg";
	exts[1] = ".png";
	exts[2] = ".bmp";
	exts[3] = ".tif";
	return exts;
}

vector<string> getAvailableMusicExts() {
	vector<string> exts(3);
	exts[0] = ".wav";
	exts[1] = ".mp3";
	exts[2] = ".ogg";
	return exts;
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

string getRendererName(int id) {
	SDL_RendererInfo info;
	if (!SDL_GetRenderDriverInfo(id, &info))
		SDL_GetRenderDriverInfo(-1, &info);
	return info.name;
}
