#pragma once

#include "utils/objects.h"
#include "utils/types.h"
#include "prog/program.h"
#include "prog/library.h"

class Scene {
public:
	Scene();
	~Scene();
	
	void SwitchMenu(EMenu newMenu, void* dat=nullptr);
	void OnMouseButton();

	Program* getProgram() const;
	const vector<Object*>& Objects() const;
	Object* FocusedObject() const;
	
private:
	kptr<Program> program;
	kptr<Library> library;
	vector<Object*> objects;
	uint focObject;

	bool CheckButtonClick(Button* obj);
	bool CheckListBoxClick(ListBox* obj);
	bool CheckTileBoxClick(TileBox* obj);

};
