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
	void OnMouseDown(const vec2i& mPos, ClickType click);
	void OnMouseUp(const vec2i& mPos, ClickType click);
	void OnMouseWheel(const vec2i& wMov);
	void OnMouseLeave();

	const GeneralSettings& Settings() const;
	void LibraryPath(const string& dir);
	void PlaylistsPath(const string& dir);

	Program* getProgram();
	Library* getLibrary();
	const vector<Object*>& Objects() const;
	Object* FocusedObject();
	bool isDraggingSlider(ScrollArea* obj) const;	// whether obj's slider is currently being dragged
	ListItem* SelectedButton();						// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)

	Popup* getPopup();
	void SetPopup(Popup* newPopup, Capturer* capture=nullptr);

private:
	Program program;
	Library library;
	GeneralSettings sets;

	vector<Object*> objects;
	kk::sptr<Popup> popup;

	Object* focObject;		// currently focused object (should be object over which the cursor is poistioned)
	ClickStamp stamp;
	bool dragSlider;		// whether a slider is currently being dragged

	void ResizeObjects(vector<Object*>& objs);

	void CheckListBoxClick(const vec2i& mPos, ListBox* obj, ClickType click);
	void CheckTableBoxClick(const vec2i& mPos, TableBox* obj, ClickType click);
	void CheckTileBoxClick(const vec2i& mPos, TileBox* obj, ClickType click);
	void CheckReaderBoxClick(const vec2i& mPos, ReaderBox* obj, ClickType click);
	void CheckReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* obj);
};
