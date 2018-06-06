#pragma once

#include "prog/defaults.h"

enum class Color : uint8 {
	background,
	normal,
	dark,
	light,
	select,
	text,
	texture,
	numColors
};

class Binding {
public:
	enum class Type : uint8 {
		enter,
		escape,
		up,
		down,
		left,
		right,
		scrollUp,
		scrollDown,
		scrollLeft,
		scrollRight,
		cursorUp,
		cursorDown,
		cursorLeft,
		cursorRight,
		centerView,
		scrollFast,
		scrollSlow,
		pageUp,
		pageDown,
		zoomIn,
		zoomOut,
		zoomReset,
		nextDir,
		prevDir,
		fullscreen,
		numBindings
	};
	enum Assignment : uint8 {
		ASG_NONE	= 0x0,
		ASG_KEY     = 0x1,
		ASG_JBUTTON = 0x2,
		ASG_JHAT    = 0x4,
		ASG_JAXIS_P = 0x8,	// use only positive values
		ASG_JAXIS_N = 0x10,	// use only negative values
		ASG_GBUTTON = 0x20,
		ASG_GAXIS_P = 0x40,
		ASG_GAXIS_N = 0x80
	};

	Binding();
	void setDefaultSelf(Type type);

	SDL_Scancode getKey() const { return key; }
	bool keyAssigned() const { return asg & ASG_KEY; }
	void clearAsgKey();
	void setKey(SDL_Scancode KEY);

	uint8 getJctID() const { return jctID; }
	bool jctAssigned() const;
	void clearAsgJct();

	bool jbuttonAssigned() const { return asg & ASG_JBUTTON; }
	void setJbutton(uint8 BUT);

	bool jaxisAssigned() const { return asg & (ASG_JAXIS_P | ASG_JAXIS_N); }
	bool jposAxisAssigned() const { return asg & ASG_JAXIS_P; }
	bool jnegAxisAssigned() const { return asg & ASG_JAXIS_N; }
	void setJaxis(uint8 AXIS, bool positive);

	uint8 getJhatVal() const { return jHatVal; }
	bool jhatAssigned() const { return asg & ASG_JHAT; }
	void setJhat(uint8 HAT, uint8 VAL);

	uint8 getGctID() const { return gctID; }
	bool gctAssigned() const { return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N); }
	void clearAsgGct();

	SDL_GameControllerButton getGbutton() const { return static_cast<SDL_GameControllerButton>(gctID); }
	bool gbuttonAssigned() const { return asg & ASG_GBUTTON; }
	void setGbutton(SDL_GameControllerButton BUT);

	SDL_GameControllerAxis getGaxis() const { return static_cast<SDL_GameControllerAxis>(gctID); }
	bool gaxisAssigned() const { return asg & (ASG_GAXIS_P | ASG_GAXIS_N); }
	bool gposAxisAssigned() const { return asg & ASG_GAXIS_P; }
	bool gnegAxisAssigned() const { return asg & ASG_GAXIS_N; }
	void setGaxis(SDL_GameControllerAxis AXIS, bool positive);

	bool isAxis() const { return callAxis; }
	void (ProgState::*(getBcall() const))() { return bcall; }
	void setBcall(void (ProgState::*call)());
	void (ProgState::*(getAcall() const))(float) { return acall; }
	void setAcall(void (ProgState::*call)(float));

private:
	Assignment asg;		// stores data for checking whether key and/or button/axis are assigned
	SDL_Scancode key;	// keybord key
	uint8 jctID;		// joystick control ID
	uint8 jHatVal;		// joystick hat value
	uint8 gctID;		// gamepad control ID

	bool callAxis;
	union {
		void (ProgState::*bcall)();
		void (ProgState::*acall)(float);
	};
};
inline Binding::Assignment operator~(Binding::Assignment a) { return static_cast<Binding::Assignment>(~static_cast<uint8>(a)); }
inline Binding::Assignment operator&(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline Binding::Assignment operator&=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline Binding::Assignment operator^(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline Binding::Assignment operator^=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline Binding::Assignment operator|(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
inline Binding::Assignment operator|=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) | static_cast<uint8>(b)); }

class Settings {
public:
	Settings(bool MAX=Default::maximized, bool FSC=Default::fullscreen, const vec2i& RES=Default::resolution, const string& THM="", const string& FNT=Default::font, const string& LANG=Default::language, const string& LIB="", const string& RNDR="", const vec2f& SSP=Default::scrollSpeed, int16 DDZ=Default::controllerDeadzone);

	string getResolutionString() const;
	void setResolution(const string& line);

	const string& getTheme() const { return theme; }
	const string& setTheme(const string& name);
	const string& getFont() const { return font; }
	string setFont(const string& newFont);			// returns path to the font file, not the name
	const string& getLang() const { return lang; }
	const string& setLang(const string& language);
	const string& getDirLib() const { return dirLib; }
	const string& setDirLib(const string& dir);

	int getRendererIndex();
	static vector<string> getAvailibleRenderers();
	static string getRendererName(int id);

	string getScrollSpeedString() const;
	void setScrollSpeed(const string& line);
	int getDeadzone() const { return deadzone; }
	void setDeadzone(int zone);

	bool maximized, fullscreen;
	vec2i resolution;
	string renderer;
	vec2f scrollSpeed;
private:
	int deadzone;
	string theme;
	string font;
	string lang;
	string dirLib;
};
