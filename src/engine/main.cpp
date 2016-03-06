#include "world.h"

int main(int argc, char** argv) {
	for (int i = 0; i != argc; i++)
		World::args.push_back(argv[i]);
	World::engine = new Engine;
	return World::engine->Run();
}