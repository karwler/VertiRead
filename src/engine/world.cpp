#include "world.h"
#ifndef WITH_ICU
#include "utils/compare.h"
#endif

template <Integer C, InvocableR<string, std::basic_string_view<C>> F>
pair<vector<string>, uset<string>> getArgs(int argc, C** argv, F conv) {
	vector<string> vals;
	uset<string> flags;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			C* pos;
			for (pos = argv[i] + 1; *pos == '-'; ++pos);
			if (*pos)
				flags.insert(conv(pos));
		} else if (C* pos = argv[i] + (argv[i][0] == '\\' && argv[i][1] == '-'); *pos)
			vals.push_back(conv(pos));
	}
	return pair(std::move(vals), std::move(flags));
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
#endif
	vector<string> vals;
	uset<string> flags;
#ifdef __MINGW32__
	if (lpCmdLine && lpCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).c_str(), &argc)) {
#else
	if (pCmdLine && pCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
			std::tie(vals, flags) = getArgs(argc, argv, swtos);
			LocalFree(argv);
		}
#else
int main(int argc, char** argv) {
	auto [vals, flags] = getArgs(argc, argv, stos);
#endif
	setlocale(LC_CTYPE, "");
#ifdef WITH_ICU
	StrNatCmp::init();
#endif
	int rc = World::winSys()->start(std::move(vals), std::move(flags));
#ifdef WITH_ICU
	StrNatCmp::free();
#endif
	return rc;
}
