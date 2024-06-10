#include "Helpers.h"

//My standard types I like to use
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

//Global vars
	extern ImGuiIO* io;
	extern GLFWwindow* glfwWindow;	//The main glfw Window object in Main.cpp

extern char szWCSChoices[9][20]; //List of all available WCSs, and which one is active

void HelpMarker(const char* sString);

void Clout_Init();
void Clout_CreateDefaultLayout();
void Clout_Shutdown();
bool Clout_MainLoop();


namespace Carvera
{
	struct Status
	{
		enum eStatus	//https://smoothieware.github.io/Webif-pack/documentation/web/html/configuring-grbl-v0.html
		{
			Idle = 0,
			Queue,
			Busy,
			Hold,
			Homing,
			Alarm
		};
	};

	struct CoordSystem
	{
		enum eCoordSystem
		{
			Unknown = 0,
			G53,
			G54,
			G55,
			G56,
			G57,
			G58,
			G59
		};
	};

	struct Positioning
	{
		enum ePositioning
		{
			Absolute = 0,
			Relative
		};
	};

	struct Units
	{
		enum eUnits
		{
			mm = 0,
			inch
		};
	};

	struct Plane
	{
		enum ePlane
		{
			XYZ = 0,
			XZY,
			YZX
		};
	};
}

struct MACHINE_STATUS
{
	Carvera::Status::eStatus Status;

	struct MACHINE_COORD
	{
		DOUBLE_XYZ Working; //Coords in current WCS
		DOUBLE_XYZ Machine; //Coords in machine system
	}Coord;

	DOUBLE_XYZ FeedRates; //Current feed rates for movement

	Carvera::CoordSystem::eCoordSystem WCS; //Current coordinate system in use
	Carvera::Positioning::ePositioning Positioning;	//Positioning mode in use
	Carvera::Units::eUnits Units;
	Carvera::Plane::ePlane Plane;

	unsigned char bCurrentTool;
	float fToolLengthOffset; //Length of the installed tool. 0 if none.
};

extern MACHINE_STATUS MachineStatus;
