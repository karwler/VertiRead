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

Library*World::library() {
	return engine->getScene()->getLibrary();
}

Program* World::program() {
	return engine->getScene()->getProgram();
}

void World::PlaySound(const string& sound) {
	if (engine->getAudioSys())
		engine->getAudioSys()->PlaySound(sound);
}
