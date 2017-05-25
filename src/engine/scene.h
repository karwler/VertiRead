#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"
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
	vector<Object*> Objects();
	Object* FocusedObject();			// returns pointer to object focused by keyboard
	ListItem* SelectedButton();	// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)
	Popup* getPopup();
	void SetPopup(Popup* box);			// use nullptr to close

private:
	Program program;
	Library library;
	GeneralSettings sets;

	vector<Object*> objects;
	kk::sptr<Popup> popup;

	btsel focObject;		// id of object currently focused by keyboard/controller
	ScrollArea* objectHold;	// pointer to object currently being dragged by mouse (nullptr if none is being held)

	void CheckObjectsClick(const vec2i& mPos, EClick clickType, bool handleHold);
	void CheckPopupClick(const vec2i& mPos);
	bool CheckSliderClick(const vec2i& mPos, ScrollArea* obj);
	void CheckListBoxClick(const vec2i& mPos, ListBox* obj, EClick clickType);
	void CheckTableBoxClick(const vec2i& mPos, TableBox* obj, EClick clickType);
	void CheckTileBoxClick(const vec2i& mPos, TileBox* obj, EClick clickType);
	void CheckReaderBoxClick(const vec2i& mPos, ReaderBox* obj, EClick clickType, bool handleHold);
	void CheckPopupSimpleClick(const vec2i& mPos, PopupMessage* obj);
	void CheckPopupChoiceClick(const vec2i& mPos, PopupChoice* obj);
};
