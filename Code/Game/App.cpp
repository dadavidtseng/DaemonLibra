//-----------------------------------------------------------------------------------------------
// App.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Game/App.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Platform/Window.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"

//-----------------------------------------------------------------------------------------------
App*                   g_theApp        = nullptr; // Created and owned by Main_Windows.cpp
AudioSystem*           g_theAudio      = nullptr; // Created and owned by the App
BitmapFont*            g_theBitmapFont = nullptr; // Created and owned by the App
Game*                  g_theGame       = nullptr; // Created and owned by the App
Renderer*              g_theRenderer   = nullptr; // Created and owned by the App
RandomNumberGenerator* g_theRNG        = nullptr; // Created and owned by the App
Window*                g_theWindow     = nullptr; // Created and owned by the App

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//-----------------------------------------------------------------------------------------------
void App::Startup()
{
    LoadGameConfig("Data/GameConfig.xml");

    // Create All Engine Subsystems
    sEventSystemConfig eventSystemConfig;
    g_eventSystem = new EventSystem(eventSystemConfig);
    g_eventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_eventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    sInputSystemConfig inputConfig;
    g_input = new InputSystem(inputConfig);

    sWindowConfig windowConfig;
    windowConfig.m_aspectRatio = 2.f;
    windowConfig.m_inputSystem = g_input;

    windowConfig.m_consoleTitle[0]  = " .---------------.   .----------------.   .----------------.   .----------------.   .----------------.\n";
    windowConfig.m_consoleTitle[1]  = "| .-------------. | | .--------------. | | .--------------. | | .--------------. | | .--------------. |\n";
    windowConfig.m_consoleTitle[2]  = "| |   _____     | | | |     _____    | | | |   ______     | | | |  _______     | | | |      __      | |\n";
    windowConfig.m_consoleTitle[3]  = "| |  |_   _|    | | | |    |_   _|   | | | |  |_   _ \\    | | | | |_   __ \\    | | | |     /  \\     | |\n";
    windowConfig.m_consoleTitle[4]  = "| |    | |      | | | |      | |     | | | |    | |_) |   | | | |   | |__) |   | | | |    / /\\ \\    | |\n";
    windowConfig.m_consoleTitle[5]  = "| |    | |   _  | | | |      | |     | | | |    |  __'.   | | | |   |  __ /    | | | |   / ____ \\   | |\n";
    windowConfig.m_consoleTitle[6]  = "| |   _| |__/ | | | | |     _| |_    | | | |   _| |__) |  | | | |  _| |  \\ \\_  | | | | _/ /    \\ \\_ | |\n";
    windowConfig.m_consoleTitle[7]  = "| |  |________| | | | |    |_____|   | | | |  |_______/   | | | | |____| |___| | | | ||____|  |____|| |\n";
    windowConfig.m_consoleTitle[8]  = "| |             | | | |              | | | |              | | | |              | | | |              | |\n";
    windowConfig.m_consoleTitle[9]  = "| '-------------' | | '--------------' | | '--------------' | | '--------------' | | '--------------' |\n";
    windowConfig.m_consoleTitle[10] = " '---------------'   '----------------'   '----------------'   '----------------'   '----------------'\n";

    windowConfig.m_windowTitle = "SD1-A8: Epilogue";
    g_theWindow                = new Window(windowConfig);

    sRendererConfig renderConfig;
    renderConfig.m_window = g_theWindow;
    g_theRenderer         = new Renderer(renderConfig); // Create render

    // Initialize devConsoleCamera
    m_devConsoleCamera = new Camera();

    Vec2 const  bottomLeft     = Vec2::ZERO;
    float const screenSizeX    = g_gameConfigBlackboard.GetValue("screenSizeX", -1.f);
    float const screenSizeY    = g_gameConfigBlackboard.GetValue("screenSizeY", -1.f);
    Vec2 const  screenTopRight = Vec2(screenSizeX, screenSizeY);

    m_devConsoleCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);

    sDevConsoleConfig devConsoleConfig;
    devConsoleConfig.m_defaultRenderer = g_theRenderer;
    devConsoleConfig.m_defaultFontName = "SquirrelFixedFont";
    devConsoleConfig.m_defaultCamera   = m_devConsoleCamera;
    g_devConsole                    = new DevConsole(devConsoleConfig);

    sAudioSystemConfig audioConfig;
    g_theAudio = new AudioSystem(audioConfig);

    g_eventSystem->Startup();
    g_input->Startup();
    g_theWindow->Startup();
    g_theRenderer->Startup();
    g_devConsole->StartUp();
    g_theAudio->Startup();

    g_theBitmapFont = g_theRenderer->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)
    g_theRNG        = new RandomNumberGenerator();
    g_theGame       = new Game();

    m_devConsoleCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
}

//-----------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    delete g_theGame;
    g_theGame = nullptr;

    delete g_theRNG;
    g_theRNG = nullptr;

    delete g_theBitmapFont;
    g_theBitmapFont = nullptr;

    g_theAudio->Shutdown();
    g_devConsole->Shutdown();
    g_theRenderer->Shutdown();
    g_theWindow->Shutdown();
    g_input->Shutdown();
    g_eventSystem->Shutdown();

    // Destroy all Engine Subsystem
    delete g_theAudio;
    g_theAudio = nullptr;

    delete g_devConsole;
    g_devConsole = nullptr;

    delete g_theRenderer;
    g_theRenderer = nullptr;

    delete g_theWindow;
    g_theWindow = nullptr;

    delete g_input;
    g_input = nullptr;

    delete g_eventSystem;
    g_eventSystem = nullptr;
}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void App::RunFrame()
{
    float const currentTime  = static_cast<float>(GetCurrentTimeSeconds());
    float const deltaSeconds = currentTime - m_timeLastFrameStart;
    m_timeLastFrameStart     = currentTime;

    // DebuggerPrintf("currentTime = %.06f\n", timeNow);

    BeginFrame();         // Engine pre-frame stuff
    Update(deltaSeconds); // Game updates / moves / spawns / hurts / kills stuff
    Render();             // Game draws current state of things
    EndFrame();           // Engine post-frame stuff
}

//-----------------------------------------------------------------------------------------------
void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!m_isQuitting)
    {
        // Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
        RunFrame();
    }
}

//----------------------------------------------------------------------------------------------------
STATIC bool App::OnCloseButtonClicked(EventArgs& args)
{
    UNUSED(args)

    RequestQuit();

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC void App::RequestQuit()
{
    m_isQuitting = true;
}

//-----------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
    g_eventSystem->BeginFrame();
    g_input->BeginFrame();
    g_theWindow->BeginFrame();
    g_theRenderer->BeginFrame();
    g_devConsole->BeginFrame();
    g_theAudio->BeginFrame();
    // g_theNetwork->BeginFrame();
    // g_theWindow->BeginFrame();
}

//-----------------------------------------------------------------------------------------------
void App::Update(const float deltaSeconds)
{
    Clock::TickSystemClock();

    if (g_theGame->IsMarkedForDelete()) DeleteAndCreateNewGame();

    UpdateFromController();
    UpdateFromKeyBoard();
    g_theGame->Update(deltaSeconds);
}

//-----------------------------------------------------------------------------------------------
// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
//
// Ultimately this function (App::Render) will only call methods on Renderer (like Renderer::DrawVertexArray)
//	to draw things, never calling OpenGL (nor DirectX) functions directly.
//
void App::Render() const
{
    g_theRenderer->ClearScreen(Rgba8::BLACK);
    g_theGame->Render();

    g_devConsole->Render(AABB2(Vec2::ZERO, Vec2(1600.f, 30.f)));
}

//-----------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_eventSystem->EndFrame();
    g_input->EndFrame();
    g_theWindow->EndFrame();
    g_theRenderer->EndFrame();
    g_devConsole->EndFrame();
    g_theAudio->EndFrame();
}

//-----------------------------------------------------------------------------------------------
void App::UpdateFromKeyBoard()
{
    if (g_input->WasKeyJustPressed(KEYCODE_ESC))
    {
        if (g_theGame->IsAttractMode()) RequestQuit();
    }

    if (g_input->WasKeyJustPressed(KEYCODE_F8))
    {
        if (!g_theGame->IsAttractMode())
        {
            DeleteAndCreateNewGame();
        }
    }
}

//-----------------------------------------------------------------------------------------------
void App::UpdateFromController()
{
    XboxController const& controller = g_input->GetController(0);

    if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
    {
        if (g_theGame->IsAttractMode()) RequestQuit();
    }

    if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_RIGHT))
    {
        if (!g_theGame->IsAttractMode())
        {
            DeleteAndCreateNewGame();
        }
    }
}

//-----------------------------------------------------------------------------------------------
void App::DeleteAndCreateNewGame()
{
    delete g_theGame;
    g_theGame = nullptr;

    g_theGame = new Game();
}

//----------------------------------------------------------------------------------------------------
void App::LoadGameConfig(char const* gameConfigXmlFilePath)
{
    XmlDocument     gameConfigXml;
    XmlResult const result = gameConfigXml.LoadFile(gameConfigXmlFilePath);

    if (result == XmlResult::XML_SUCCESS)
    {
        if (XmlElement const* rootElement = gameConfigXml.RootElement())
        {
            g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
        }
        else
        {
            printf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigXmlFilePath);
        }
    }
    else
    {
        printf("WARNING: failed to load game config from file \"%s\"\n", gameConfigXmlFilePath);
    }
}
