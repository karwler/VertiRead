#include "world.h"
#include <clocale>
#include <locale>

WindowSys World::windowSys;
vector<string> World::argVals;
uset<string> World::argFlags;
umap<string, string> World::argOpts;

#if defined(_WIN32) && !defined(_DEBUG)
void World::init(wchar* argstr) {
	int argc;
	LPWSTR* argv = CommandLineToArgvW(argstr, &argc);
	setArgs(0, argc, argv);
	LocalFree(argv);
}
#endif

#ifdef _WIN32
void World::setArgs(int i, int argc, wchar** argv) {
	for (; i < argc; i++) {
		if (argv[i][0] == '-') {
			wchar* flg = argv[i] + (argv[i][1] == '-' ? 2 : 1);
			if (int ni = i + 1; ni < argc && argv[ni][0] != '-')
				argOpts.emplace(wtos(flg), wtos(argv[++i]));
			else
				argFlags.emplace(wtos(flg));
		} else
			argVals.emplace_back(wtos(argv[i]));
	}
}
#else
void World::setArgs(int i, int argc, char** argv) {
	for (; i < argc; i++) {
		if (argv[i][0] == '-') {
			char* flg = argv[i] + (argv[i][1] == '-' ? 2 : 1);
			if (int ni = i + 1; ni < argc && argv[ni][0] != '-')
				argOpts.emplace(flg, argv[++i]);
			else
				argFlags.emplace(flg);
		} else
			argVals.emplace_back(argv[i]);
	}
}
#endif

#ifdef _WIN32
#ifdef _DEBUG
int wmain(int argc, wchar** argv) {
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#endif
#else
int main(int argc, char** argv) {
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
#if defined(_WIN32) && !defined(_DEBUG)
	World::init(pCmdLine);
#else
	World::init(argc, argv);
#endif
	return World::winSys()->start();
}
