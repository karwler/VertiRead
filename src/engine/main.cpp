#include "world.h"
#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_WIN32) && !defined(_DEBUG)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::args = getWords(wtos(pCmdLine), ' ');
#else
int main(int argc, char** argv) {
	World::args.resize(argc-1);
	for (int i=1; i!=argc; i++)
		World::args[i-1] = argv[i];
#endif
	int retval = 0;
	try {
		World::engine()->start();
	} catch (Exception exc) {
		exc.printMessage();
		retval = exc.retval;
	} catch (std::logic_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -3;
	} catch (std::runtime_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -2;
	} catch (...) {
		cerr << "unknown error" << endl;
		retval = -1;
	}
	World::engine()->cleanup();
	return retval;
}
