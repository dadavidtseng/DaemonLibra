//-----------------------------------------------------------------------------------------------
// App.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Game/App.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"


//-----------------------------------------------------------------------------------------------
App*  g_app  = nullptr; // Created and owned by Main_Windows.cpp
Game* g_game = nullptr; // Created and owned by the App

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
App::App()
{
    GEngine::Get().Construct();
}

//----------------------------------------------------------------------------------------------------
App::~App()
{
    GEngine::Get().Destruct();
}

//-----------------------------------------------------------------------------------------------
void App::Startup()
{
    GEngine::Get().Startup();

    LoadGameConfig("Data/GameConfig.xml");

    g_eventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_eventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    g_game = new Game();
}

//-----------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    GAME_SAFE_RELEASE(g_game);

    g_eventSystem->UnsubscribeEventCallbackFunction("quit", OnCloseButtonClicked);
    g_eventSystem->UnsubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);

    GEngine::Get().Shutdown();
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
    g_window->BeginFrame();
    g_renderer->BeginFrame();
    g_devConsole->BeginFrame();
    g_input->BeginFrame();
    g_audio->BeginFrame();
}

//-----------------------------------------------------------------------------------------------
void App::Update(const float deltaSeconds)
{
    Clock::TickSystemClock();

    if (g_game->IsMarkedForDelete()) DeleteAndCreateNewGame();

    UpdateFromController();
    UpdateFromKeyBoard();
    g_game->Update(deltaSeconds);
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
    g_renderer->ClearScreen(Rgba8::BLACK);
    g_game->Render();

    AABB2 const box            = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_devConsole->Render(box);
}

//-----------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_eventSystem->EndFrame();
    g_input->EndFrame();
    g_window->EndFrame();
    g_renderer->EndFrame();
    g_devConsole->EndFrame();
    g_audio->EndFrame();
}

//-----------------------------------------------------------------------------------------------
void App::UpdateFromKeyBoard()
{
    if (g_input->WasKeyJustPressed(KEYCODE_ESC))
    {
        if (g_game->IsAttractMode()) RequestQuit();
    }

    if (g_input->WasKeyJustPressed(KEYCODE_F8))
    {
        if (!g_game->IsAttractMode())
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
        if (g_game->IsAttractMode()) RequestQuit();
    }

    if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_RIGHT))
    {
        if (!g_game->IsAttractMode())
        {
            DeleteAndCreateNewGame();
        }
    }
}

//-----------------------------------------------------------------------------------------------
void App::DeleteAndCreateNewGame()
{
    delete g_game;
    g_game = nullptr;

    g_game = new Game();
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
