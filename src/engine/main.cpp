#include "world.h"

#if defined(_WIN32) && !defined(_DEBUG)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	World::args = getWords(pCmdLine, , ' ', ' ');
#else
int main(int argc, char** argv) {
	for (int i=0; i!=argc; i++)
		World::args.push_back(argv[i]);
#endif
	int retval = 0;
	try {
		World::engine()->Run();
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
	World::engine()->Cleanup();
	return retval;
}
