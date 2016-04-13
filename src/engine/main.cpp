#include "world.h"

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::args = getWords(wtos(pCmdLine), false);
#else
int main(int argc, char** argv) {
	for (int i=0; i!=argc; i++)
		World::args.push_back(argv[i]);
#endif
	int retval = 0;
	try {
		World::engine = new Engine;
		World::engine->Run();
	}
	catch (Exception exc) {
		exc.Display();
		retval = exc.retval;
	}
	catch (std::logic_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -3;
	}
	catch (std::runtime_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -2;
	}
	catch (...) {
		cerr << "unknown error" << endl;
		retval = -1;
	}
	World::engine->Cleanup();
	return retval;
}
