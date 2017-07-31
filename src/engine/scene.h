#pragma once

#include "utils/scrollAreas.h"
#include "utils/capturers.h"
#include "prog/library.h"
#include "prog/program.h"

class Scene {
public:
	Scene(const GeneralSettings& SETS=GeneralSettings());
	~Scene();
	
	void SwitchMenu(const vector<Object*>& objs);
	void ResizeMenu();
	void Tick(float dSec);
	void OnMouseMove(const vec2i& mPos, const vec2i& mMov);
	void OnMouseDown(const vec2i& mPos, EClick clickType, bool handleHold=true);
	void OnMouseUp();
	void OnMouseWheel(const vec2i& wMov);

	const GeneralSettings& Settings() const;
	void LibraryPath(const string& dir);
	void PlaylistsPath(const string& dir);

	Program* getProgram();
	Library* getLibrary();
	const vector<Object*>& Objects() const;
	Object* FocusedObject();
	ListItem* SelectedButton();	// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)

	const Popup* getPopup() const;
	void SetPopupMessage(const string& msg);
	void SetPopupChoice(const string& msg, void (Program::*callb)());
	void SetPopupText(const string& msg, const string& text, void (Program::*callt)(const string&), void (Program::*callb)());
	void DelPopup();

private:
	Program program;
	Library library;
	GeneralSettings sets;

	vector<Object*> objects;
	kk::sptr<Popup> popup;
	Object* focObject;			// currently focused object (should be object over which the cursor is poistioned)
	ScrollArea* objectHold;		// pointer to object currently being dragged by mouse (nullptr if none is being held)

	void ResizeObjects(vector<Object*>& objs);
	void MouseMoveObjectOverCheck(vector<Object*>& objs, const vec2i& mPos);	// return true if found an object

	bool CheckSliderClick(const vec2i& mPos, ScrollArea* obj);
	void CheckListBoxClick(const vec2i& mPos, ListBox* obj, EClick clickType);
	void CheckTableBoxClick(const vec2i& mPos, TableBox* obj, EClick clickType);
	void CheckTileBoxClick(const vec2i& mPos, TileBox* obj, EClick clickType);
	void CheckReaderBoxClick(const vec2i& mPos, ReaderBox* obj, EClick clickType, bool handleHold);
};
