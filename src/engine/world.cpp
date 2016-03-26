#include "world.h"

kptr<Engine> World::engine;
vector<string> World::args;

AudioSys* World::audioSys() {
	return engine->getAudioSys();
}

InputSys* World::inputSys() {
	return engine->getInputSys();
}

WindowSys* World::winSys() {
	return engine->getWindowSys();
}

Scene* World::scene() {
	return engine->getScene();
}

Program* World::program() {
	return engine->getScene()->getProgram();
}
