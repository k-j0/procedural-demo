
#define SHOW_CONSOLE //if defined, opens a console for stdout

#ifdef SHOW_CONSOLE
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "System.h"
#include "App.h"
#include <random>
#include <ctime>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow) {

#ifdef SHOW_CONSOLE
	//open console
	AllocConsole();
	std::freopen("conin$", "r", stdin);
	std::freopen("conout$", "w", stdout);
	std::freopen("conout$", "w", stderr);
#endif

	srand(time(0));

	//Initialize system
	BaseApplication* app = new App();
	System* system = new System(app, 1200, 675, true, false);

	//Run app
	system->run();

	//Exit
	delete system;
	system = 0;

#ifdef SHOW_CONSOLE
	FreeConsole();
#endif

	return 0;
}