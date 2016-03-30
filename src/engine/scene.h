#pragma once

#include "utils/scrollAreas.h"
#include "prog/program.h"

class Scene {
public:
	Scene();
	~Scene();
	
	void SwitchMenu(EMenu newMenu, void* dat=nullptr);
	void ResizeMenu();
	void Tick();
	void OnMouseDown();
	void OnMouseUp();
	void OnMouseWheel(int ymov);

	Program* getProgram() const;
	const vector<Object*>& Objects() const;
	Object* FocusedObject() const;			// returns pointer to object focused by keyboard
	
private:
	kptr<Program> program;
	vector<Object*> objects;

	uint focObject;			// id of object currently focused by keyboard
	ScrollArea* objectHold;	// pointer to object currently being dragged by mouse (nullptr if none is being held)

	bool CheckSliderClick(ScrollArea* obj);
	bool CheckListBoxClick(ListBox* obj);
	bool CheckTileBoxClick(TileBox* obj);
	bool CheckReaderBoxClick(ReaderBox* obj);
};
