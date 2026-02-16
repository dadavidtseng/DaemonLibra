//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
class App
{
public:
    App()  = default;
    ~App() = default;
    void Startup();
    void Shutdown();
    void RunFrame();

    void RunMainLoop();

    static bool OnCloseButtonClicked(EventArgs& args);
    static void RequestQuit();
    static bool m_isQuitting;

private:
    void BeginFrame() const;
    void Update(float deltaSeconds);
    void Render() const;
    void EndFrame() const;

    void UpdateFromController();
    void UpdateFromKeyBoard();

    void DeleteAndCreateNewGame();
    void LoadGameConfig(char const* gameConfigXmlFilePath);

    float m_timeLastFrameStart = 0.f;

    Camera* m_devConsoleCamera = nullptr;
};
