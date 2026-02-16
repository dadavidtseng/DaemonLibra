//----------------------------------------------------------------------------------------------------
// Game.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Game.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"


//----------------------------------------------------------------------------------------------------
Game::Game()
{
    InitializeTiles();
    InitializeMaps();
    InitializeAudio();

    m_worldCamera  = new Camera();
    m_screenCamera = new Camera();

    Vec2 const  playerTankInitPosition           = g_gameConfigBlackboard.GetValue("playerTankInitPosition", Vec2(2.f, 2.f));
    float const playerTankInitOrientationDegrees = g_gameConfigBlackboard.GetValue("playerTankInitOrientationDegrees", 30.f);


    m_playerTank = dynamic_cast<PlayerTank*>(m_currentMap->SpawnNewEntity(ENTITY_TYPE_PLAYER_TANK,
                                                                          ENTITY_FACTION_GOOD,
                                                                          playerTankInitPosition,
                                                                          playerTankInitOrientationDegrees));

    Vec2 const  bottomLeft  = Vec2::ZERO;
    float const worldSizeX  = g_gameConfigBlackboard.GetValue("worldSizeX", 16.f);
    float const worldSizeY  = g_gameConfigBlackboard.GetValue("worldSizeY", 8.f);
    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_worldCamera->SetOrthoGraphicView(bottomLeft, Vec2(worldSizeX, worldSizeY));
    m_screenCamera->SetOrthoGraphicView(bottomLeft, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);

    m_attractModePlayback = g_audio->StartSound(m_attractModeBgm, true, 3, 0, 1, false);
}

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
    delete m_currentMap;
    m_currentMap = nullptr;

    m_maps.clear();

    delete m_playerTank;
    m_playerTank = nullptr;

    delete m_screenCamera;
    m_screenCamera = nullptr;

    delete m_worldCamera;
    m_worldCamera = nullptr;

    g_audio->StopSound(m_InGamePlayback);
    g_audio->StopSound(m_attractModePlayback);
    g_audio->StopSound(m_gameWinPlayback);
    g_audio->StopSound(m_gameLosePlayback);
}

//-----------------------------------------------------------------------------------------------
void Game::Update(float deltaSeconds)
{
    // #TODO: Select keyboard or controller
    UpdateMarkForDelete();
    UpdateFromKeyBoard();
    UpdateFromController();
    UpdateCamera(deltaSeconds);
    UpdateAttractMode(deltaSeconds);
    AdjustForPauseAndTimeDistortion(deltaSeconds);

    if (m_playerTank->m_isDead)
    {
        m_gameOverCountDown -= deltaSeconds;
    }

    if (m_playerTank->m_isDead &&
        m_gameOverCountDown <= 0.f)
    {
        m_isPaused          = true;
        m_isGameLoseMode    = true;
        m_gameOverCountDown = 3.f;
        g_audio->StopSound(m_InGamePlayback);

        if (m_gameLosePlayback == 0) m_gameLosePlayback = g_audio->StartSound(m_gameLoseBgm);
    }

    if (m_currentMap->GetTileCoordsFromWorldPos(m_playerTank->m_position).x == m_currentMap->GetMapExitPosition().x &&
        m_currentMap->GetTileCoordsFromWorldPos(m_playerTank->m_position).y == m_currentMap->GetMapExitPosition().y)
    {
        if (m_currentMap->GetMapIndex() == 2)
        {
            m_isPaused      = true;
            m_isGameWinMode = true;
            g_audio->StopSound(m_InGamePlayback);

            if (m_gameWinPlayback == 0) m_gameWinPlayback = g_audio->StartSound(m_gameWinBgm);

            return;
        }

        UpdateCurrentMap();
    }

    if (m_isUpdateMapCountingDown)
    {
        m_updateMapCountDown -= deltaSeconds;

        if (m_updateMapCountDown <= 0)
        {
            UpdateCurrentMap();
            m_updateMapCountDown      = 1.f;
            m_isUpdateMapCountingDown = false;
            return;
        }
    }


    if (m_currentMap) m_currentMap->Update(deltaSeconds);

    if (g_input->WasKeyJustPressed(KEYCODE_TILDE))
    {
        g_devConsole->ToggleMode(OPEN_FULL);
    }

    if (g_input->WasKeyJustPressed(KEYCODE_ENTER))
    {
        g_devConsole->AddLine(Rgba8::WHITE, "SHOOT");
        g_devConsole->Execute("SHOOT");
        // EventArgs args;
        // args.SetValue("SHOOT", "SHOOT");
        // g_theEventSystem->FireEvent("SHOOT", args);
    }
}

//-----------------------------------------------------------------------------------------------
void Game::Render() const
{
    g_renderer->BeginCamera(*m_worldCamera);

    m_currentMap->Render();
    m_currentMap->DebugRender();

    g_renderer->EndCamera(*m_worldCamera);
    //-----------------------------------------------------------------------------------------------
    g_renderer->BeginCamera(*m_screenCamera);

    RenderAttractMode();
    RenderUI();

    m_currentMap->RenderTileHeatMapText();

    g_renderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void Game::InitializeMaps()
{
    printf("( Game ) Start  | InitializeMaps\n");

    MapDefinition::InitializeMapDefs();

    m_maps.reserve(3);

    for (int mapIndex = 0; mapIndex < 3; ++mapIndex)
    {
        m_maps.push_back(new Map(*MapDefinition::s_mapDefinitions[mapIndex]));
    }

    m_currentMap = m_maps[0];

    printf("( Game ) Finish | InitializeMaps\n");
}


//----------------------------------------------------------------------------------------------------
void Game::InitializeTiles()
{
    printf("( Game ) Start  | InitializeTiles\n");

    Texture const* const tileTexture  = g_resourceSubsystem->CreateOrGetTextureFromFile(TILE_TEXTURE_IMG);
    IntVec2 const        spriteCoords = IntVec2(8, 8);
    m_tileSpriteSheet                 = new SpriteSheet(*tileTexture, spriteCoords);

    TileDefinition::InitializeTileDefs(*m_tileSpriteSheet);

    printf("( Game ) Finish | InitializeTiles\n");
}

//----------------------------------------------------------------------------------------------------
void Game::InitializeAudio()
{
    printf("( Game ) Start  | InitializeAudio\n");

    m_attractModeBgm       = g_audio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("attractModeBgm", "Data/Audios/AttractModeBgm.mp3"), eAudioSystemSoundDimension::Sound2D);
    m_InGameBgm            = g_audio->CreateOrGetSound(IN_GAME_BGM, eAudioSystemSoundDimension::Sound2D);
    m_gameWinBgm           = g_audio->CreateOrGetSound(GAME_WIN_BGM, eAudioSystemSoundDimension::Sound2D);
    m_gameLoseBgm          = g_audio->CreateOrGetSound(GAME_LOSE_BGM, eAudioSystemSoundDimension::Sound2D);
    m_clickSound           = g_audio->CreateOrGetSound(CLICK_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_pauseSound           = g_audio->CreateOrGetSound(PAUSE_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_resumeSound          = g_audio->CreateOrGetSound(RESUME_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_playerTankShootSound = g_audio->CreateOrGetSound(PLAYER_TANK_SHOOT_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_playerTankHitSound   = g_audio->CreateOrGetSound(PLAYER_TANK_HIT_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_enemyDiedSound       = g_audio->CreateOrGetSound(ENEMY_DIED_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_enemyHitSound        = g_audio->CreateOrGetSound(ENEMY_HIT_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_enemyShootSound      = g_audio->CreateOrGetSound(ENEMY_SHOOT_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_exitMapSound         = g_audio->CreateOrGetSound(EXIT_MAP_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_bulletBounceSound    = g_audio->CreateOrGetSound(BULLET_BOUNCE_SOUND, eAudioSystemSoundDimension::Sound2D);
    m_enemyDiscoverSound   = g_audio->CreateOrGetSound(ENEMY_DISCOVER_SOUND, eAudioSystemSoundDimension::Sound2D);

    printf("( Game ) Finish | InitializeAudio\n");
}

//-----------------------------------------------------------------------------------------------
void Game::UpdateMarkForDelete()
{
    XboxController const& controller = g_input->GetController(0);

    if (m_isPaused &&
        !m_isMarkedForDelete &&
        !m_isAttractMode)
    {
        if (g_input->WasKeyJustPressed(KEYCODE_ESC) ||
            controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
        {
            m_isMarkedForDelete = true;
            m_isAttractMode     = true;
            g_audio->StopSound(m_InGamePlayback);
            g_audio->StartSound(m_clickSound);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromKeyBoard()
{
    // AttractMode
    if (m_isAttractMode)
    {
        if (g_input->WasKeyJustPressed(KEYCODE_P))
        {
            m_isAttractMode = false;
            m_isPaused      = false;
            g_audio->StopSound(m_attractModePlayback);
            m_InGamePlayback = g_audio->StartSound(m_InGameBgm, true, 1, 0, m_InGameBgmSpeed, false);
            g_audio->StartSound(m_clickSound);
            return;
        }
    }

    // InGameMode
    if (!m_isAttractMode)
    {
        // GameWinMode
        if (m_isGameWinMode)
        {
            if (g_input->WasKeyJustPressed(KEYCODE_ESC))
            {
                m_isGameWinMode     = false;
                m_isMarkedForDelete = true;
                g_audio->StopSound(m_gameWinPlayback);
                g_audio->StartSound(m_attractModeBgm);
                g_audio->StartSound(m_clickSound);
            }
        }

        // GameLoseMode
        if (m_isGameLoseMode)
        {
            if (g_input->WasKeyJustPressed(KEYCODE_ESC))
            {
                m_isGameLoseMode    = false;
                m_isMarkedForDelete = true;
                g_audio->StopSound(m_gameLosePlayback);
                g_audio->StartSound(m_attractModeBgm);
                g_audio->StartSound(m_clickSound);
            }

            if (g_input->WasKeyJustPressed(KEYCODE_N))
            {
                m_playerTank->m_health = g_gameConfigBlackboard.GetValue("playerTankInitHealth", 100);
                m_playerTank->m_isDead = false;
                m_isGameLoseMode       = false;
                m_isPaused             = false;
            }
        }

        if (g_input->WasKeyJustPressed(KEYCODE_F1))
        {
            m_isDebugRendering = !m_isDebugRendering;
        }

        if (g_input->WasKeyJustPressed(KEYCODE_F3))
        {
            m_isNoClip = !m_isNoClip;
        }

        if (g_input->WasKeyJustPressed(KEYCODE_F4))
        {
            m_isDebugCamera = !m_isDebugCamera;
        }

        if (g_input->WasKeyJustPressed(KEYCODE_F9))
        {
            if (m_currentMap->GetMapIndex() == 2)
            {
                m_isPaused      = true;
                m_isGameWinMode = true;
                g_audio->StopSound(m_InGamePlayback);
                m_gameWinPlayback = g_audio->StartSound(m_gameWinBgm);

                return;
            }

            // UpdateCurrentMap();
            m_isUpdateMapCountingDown = true;
        }

        if (g_input->WasKeyJustPressed(KEYCODE_P))
        {
            if (!m_isPaused && !m_isGameWinMode && !m_isGameLoseMode)
            {
                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
            else if (m_isPaused && !m_isGameWinMode && !m_isGameLoseMode)
            {
                m_isPaused = false;
                g_audio->StartSound(m_resumeSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
            }
        }

        if (g_input->WasKeyJustPressed(KEYCODE_O))
        {
            if (!m_isPaused)
            {
                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
            else if (m_isPaused)
            {
                m_currentMap->Update(1.f / 60.f);
                m_playerTank->Update(1.f / 60.f);
            }
        }

        if (g_input->WasKeyJustPressed(KEYCODE_ESC))
        {
            if (!m_isPaused && !m_isMarkedForDelete)
            {
                if (m_isGameWinMode) return;
                if (m_isGameLoseMode) return;

                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
        }

        if (g_input->WasKeyJustPressed(KEYCODE_T))
        {
            m_isSlowMo = true;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.1f);
        }

        if (g_input->WasKeyJustReleased(KEYCODE_T))
        {
            m_isSlowMo = false;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
        }

        if (g_input->WasKeyJustPressed(KEYCODE_Y))
        {
            m_isFastMo = true;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 4.0f);
        }

        if (g_input->WasKeyJustReleased(KEYCODE_Y))
        {
            m_isFastMo = false;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromController()
{
    XboxController const& controller = g_input->GetController(0);

    // AttractMode
    if (m_isAttractMode)
    {
        if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
        {
            m_isAttractMode = false;
            m_isPaused      = false;
            g_audio->StopSound(m_attractModePlayback);
            m_InGamePlayback = g_audio->StartSound(m_InGameBgm, true, 1, 0, m_InGameBgmSpeed, false);
            g_audio->StartSound(m_clickSound);
            return;
        }
    }

    // InGameMode
    if (!m_isAttractMode)
    {
        // GameWinMode
        if (m_isGameWinMode)
        {
            if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
            {
                m_isGameWinMode     = false;
                m_isMarkedForDelete = true;
                g_audio->StopSound(m_gameWinPlayback);
                g_audio->StartSound(m_attractModeBgm);
                g_audio->StartSound(m_clickSound);
            }
        }

        // GameLoseMode
        if (m_isGameLoseMode)
        {
            if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
            {
                m_isGameLoseMode    = false;
                m_isMarkedForDelete = true;
                g_audio->StopSound(m_gameLosePlayback);
                g_audio->StartSound(m_attractModeBgm);
                g_audio->StartSound(m_clickSound);
            }

            if (controller.WasButtonJustPressed(XBOX_BUTTON_A))
            {
                m_playerTank->m_health = g_gameConfigBlackboard.GetValue("playerTankInitHealth", 100);
                m_playerTank->m_isDead = false;
                m_isGameLoseMode       = false;
                m_isPaused             = false;
            }
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_UP))
        {
            m_isDebugRendering = !m_isDebugRendering;
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_DOWN))
        {
            m_isNoClip = !m_isNoClip;
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_LSHOULDER))
        {
            m_isDebugCamera = !m_isDebugCamera;
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_B))
        {
            if (m_currentMap->GetMapIndex() == 2)
            {
                m_isPaused      = true;
                m_isGameWinMode = true;
                g_audio->StopSound(m_InGamePlayback);
                m_gameWinPlayback = g_audio->StartSound(m_gameWinBgm);

                return;
            }

            UpdateCurrentMap();
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
        {
            if (!m_isPaused && !m_isGameWinMode && !m_isGameLoseMode)
            {
                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
            else if (m_isPaused && !m_isGameWinMode && !m_isGameLoseMode)
            {
                m_isPaused = false;
                g_audio->StartSound(m_resumeSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
            }
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_RSHOULDER))
        {
            if (!m_isPaused)
            {
                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
            else if (m_isPaused)
            {
                m_currentMap->Update(1.f / 60.f);
                m_playerTank->Update(1.f / 60.f);
            }
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
        {
            if (!m_isPaused && !m_isMarkedForDelete)
            {
                if (m_isGameWinMode) return;
                if (m_isGameLoseMode) return;

                m_isPaused = true;
                g_audio->StartSound(m_pauseSound, false, 1, 0, 1, false);
                g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.f);
            }
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_RTHUMB))
        {
            m_isSlowMo = true;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 0.1f);
        }

        if (controller.WasButtonJustReleased(XBOX_BUTTON_RTHUMB))
        {
            m_isSlowMo = false;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
        }

        if (controller.WasButtonJustPressed(XBOX_BUTTON_LTHUMB))
        {
            m_isFastMo = true;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 4.0f);
        }

        if (controller.WasButtonJustReleased(XBOX_BUTTON_LTHUMB))
        {
            m_isFastMo = false;
            g_audio->SetSoundPlaybackSpeed(m_InGamePlayback, 1.f);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateCurrentMap()
{
    int const   currentMapIndex                  = m_currentMap->GetMapIndex();
    Vec2 const  playerTankInitPosition           = g_gameConfigBlackboard.GetValue("playerTankInitPosition", Vec2(2.f, 2.f));
    float const playerTankInitOrientationDegrees = g_gameConfigBlackboard.GetValue("playerTankInitOrientationDegrees", 30.f);

    m_currentMap->RemoveEntityFromMap(m_playerTank);
    m_currentMap = m_maps[currentMapIndex + 1];
    m_currentMap->AddEntityToMap(m_playerTank,
                                 playerTankInitPosition,
                                 playerTankInitOrientationDegrees);
    m_playerTank->SetBodyScale(0);

    g_audio->StartSound(m_exitMapSound);
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateCamera(float const deltaSeconds) const
{
    UNUSED(deltaSeconds)

    float const worldSizeX   = g_gameConfigBlackboard.GetValue("worldSizeX", 16.f);
    float const worldSizeY   = g_gameConfigBlackboard.GetValue("worldSizeY", 8.f);
    float const worldCenterX = g_gameConfigBlackboard.GetValue("worldCenterX", 8.f);
    float const worldCenterY = g_gameConfigBlackboard.GetValue("worldCenterY", 4.f);

    if (!m_playerTank) return;

    const Vec2      playerTankPosition = m_playerTank->m_position;
    constexpr float mapMinX            = 0.f;
    constexpr float mapMinY            = 0.f;
    const float     mapMaxX            = static_cast<float>(m_currentMap->GetMapDimension().x);
    const float     mapMaxY            = static_cast<float>(m_currentMap->GetMapDimension().y);

    Vec2 cameraMin = Vec2(playerTankPosition.x - worldCenterX, playerTankPosition.y - worldCenterY);
    Vec2 cameraMax = Vec2(playerTankPosition.x + worldCenterX, playerTankPosition.y + worldCenterY);

    cameraMin.x = RangeMapClamped(cameraMin.x, mapMinX, mapMaxX - worldSizeX, mapMinX, mapMaxX - worldSizeX);
    cameraMax.x = RangeMapClamped(cameraMax.x, mapMinX + worldSizeX, mapMaxX, mapMinX + worldSizeX, mapMaxX);

    cameraMin.y = RangeMapClamped(cameraMin.y, mapMinY, mapMaxY - worldSizeY, mapMinY, mapMaxY - worldSizeY);
    cameraMax.y = RangeMapClamped(cameraMax.y, mapMinY + worldSizeY, mapMaxY, mapMinY + worldSizeY, mapMaxY);

    m_worldCamera->SetOrthoGraphicView(cameraMin, cameraMax);

    if (m_isDebugCamera)
    {
        Vec2 const bottomLeft = Vec2::ZERO;

        float const mapWidth  = static_cast<float>(m_currentMap->GetMapDimension().x);
        float const mapHeight = static_cast<float>(m_currentMap->GetMapDimension().y);

        float aspectRatio = worldSizeX / worldSizeY;
        float newScreenX, newScreenY;

        if (mapWidth / mapHeight > aspectRatio)
        {
            newScreenX = mapWidth;
            newScreenY = mapWidth / aspectRatio;
        }
        else
        {
            newScreenX = mapHeight * aspectRatio;
            newScreenY = mapHeight;
        }

        m_worldCamera->SetOrthoGraphicView(bottomLeft, Vec2(newScreenX, newScreenY));
    }
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateAttractMode(float const deltaSeconds)
{
    if (!m_isAttractMode) return;

    if (m_glowIncreasing)
    {
        m_glowIntensity += deltaSeconds;

        if (m_glowIntensity >= 1.f)
        {
            m_glowIntensity  = 1.f;
            m_glowIncreasing = false;
        }
    }
    else
    {
        m_glowIntensity -= deltaSeconds;

        if (m_glowIntensity <= 0.f)
        {
            m_glowIntensity  = 0.f;
            m_glowIncreasing = true;
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Game::AdjustForPauseAndTimeDistortion(float& deltaSeconds) const
{
    if (m_isAttractMode) return;

    if (m_isPaused) deltaSeconds = 0.f;

    if (m_isSlowMo) deltaSeconds *= 1 / 10.f;

    if (m_isFastMo) deltaSeconds *= 4.f;
}

//----------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
    if (!m_isAttractMode) return;

    DebugDrawGlowCircle(Vec2(800, 400),
                        400.0f,
                        Rgba8::RED,
                        m_glowIntensity);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderUI() const
{
    float const screenSizeX   = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY   = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const screenCenterX = g_gameConfigBlackboard.GetValue("screenCenterX", 800.f);
    float const screenCenterY = g_gameConfigBlackboard.GetValue("screenCenterY", 400.f);

    if (m_isAttractMode) return;

    if (m_isPaused)
    {
        DebugDrawGlowBox(Vec2(screenCenterX, screenCenterY),
                         Vec2(screenSizeX, screenSizeY), Rgba8::BLACK,
                         0.5f);
    }

    if (m_isGameLoseMode)
    {
        DebugDrawGlowBox(Vec2(screenCenterX, screenCenterY),
                         Vec2(screenSizeX, screenSizeY),
                         Rgba8::RED,
                         0.5f);

        std::vector<Vertex_PCU> titleVerts;

        AddVertsForTextTriangles2D(titleVerts,
                                   "You are dead...",
                                   Vec2(30.f, screenCenterY + 100.f),
                                   48.f,
                                   Rgba8::BLACK,
                                   1.f,
                                   true,
                                   0.05f);

        AddVertsForTextTriangles2D(titleVerts,
                                   "Press \"N\" to respawn,",
                                   Vec2(30.f, screenCenterY),
                                   48.f,
                                   Rgba8::BLACK,
                                   1.f,
                                   true,
                                   0.05f);

        AddVertsForTextTriangles2D(titleVerts,
                                   "Press \"ESC\" to exit,",
                                   Vec2(30.f, screenCenterY - 100.f),
                                   48.f,
                                   Rgba8::BLACK,
                                   1.f,
                                   true,
                                   0.05f);

        g_renderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());
    }

    if (m_isGameWinMode)
    {
        DebugDrawGlowBox(Vec2(screenCenterX, screenCenterY),
                         Vec2(screenSizeX, screenSizeY),
                         Rgba8::GREEN,
                         0.5f);

        std::vector<Vertex_PCU> titleVerts;

        AddVertsForTextTriangles2D(titleVerts,
                                   "Victory!",
                                   Vec2(30.f, screenCenterY + 100.f),
                                   48.f,
                                   Rgba8::BLACK,
                                   1.f,
                                   true,
                                   0.05f);

        AddVertsForTextTriangles2D(titleVerts,
                                   "Press \"ESC\" to exit,",
                                   Vec2(30.f, screenCenterY - 100.f),
                                   48.f,
                                   Rgba8::BLACK,
                                   1.f,
                                   true,
                                   0.05f);

        g_renderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());
    }
}
