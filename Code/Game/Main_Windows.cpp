#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <cstdio>
#include <iostream>
#include "Engine/Core/EngineCommon.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(const HINSTANCE applicationInstanceHandle, HINSTANCE, const LPSTR commandLineString, int)
{
	UNUSED(applicationInstanceHandle)
	UNUSED(commandLineString)

	g_app = new App();
	g_app->Startup();
	g_app->RunMainLoop();
	g_app->Shutdown();

	GAME_SAFE_RELEASE(g_app);

	return 0;
}
