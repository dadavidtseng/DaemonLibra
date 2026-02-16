//-----------------------------------------------------------------------------------------------
// Game.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/TileDefinition.hpp"

class TileHeatMap;
//-----------------------------------------------------------------------------------------------
class Camera;
class Map;
class PlayerTank;


//-----------------------------------------------------------------------------------------------
class Game
{
public:
    Game();
    ~Game();
    void Update(float deltaSeconds);
    void Render() const;

    PlayerTank const*  GetPlayerTank() const { return m_playerTank; }
    SpriteSheet const* GetTileSpriteSheet() const { return m_tileSpriteSheet; }
    SoundID            GetPlayerTankShootSoundID() const { return m_playerTankShootSound; }
    SoundID            GetPlayerTankHitSoundID() const { return m_playerTankHitSound; }
    SoundID            GetEnemyDiedSoundID() const { return m_enemyDiedSound; }
    SoundID            GetEnemyHitSoundID() const { return m_enemyHitSound; }
    SoundID            GetEnemyShootSoundID() const { return m_enemyShootSound; }
    SoundID            GetExitMapSoundID() const { return m_exitMapSound; }
    SoundID            GetBulletBounceSoundID() const { return m_bulletBounceSound; }
    SoundID            GetEnemyDiscoverSoundID() const { return m_enemyDiscoverSound; }

    bool IsAttractMode() const { return m_isAttractMode; }
    bool IsNoClip() const { return m_isNoClip; }
    bool IsDebugRendering() const { return m_isDebugRendering; }
    bool IsMarkedForDelete() const { return m_isMarkedForDelete; }

private:
    void InitializeMaps();
    void InitializeTiles();
    void InitializeAudio();

    void UpdateMarkForDelete();
    void UpdateFromKeyBoard();
    void UpdateFromController();
    void UpdateCurrentMap();
    void UpdateCamera(float deltaSeconds) const;
    void UpdateAttractMode(float deltaSeconds);
    void AdjustForPauseAndTimeDistortion(float& deltaSeconds) const;

    void RenderAttractMode() const;
    void RenderUI() const;

    Camera* m_worldCamera             = nullptr;
    Camera* m_screenCamera            = nullptr;
    bool    m_isAttractMode           = true;
    bool    m_isGameWinMode           = false;
    bool    m_isGameLoseMode          = false;
    bool    m_isDebugRendering        = false;
    bool    m_isDebugCamera           = false;
    bool    m_isPaused                = false;
    bool    m_isSlowMo                = false;
    bool    m_isFastMo                = false;
    bool    m_isMarkedForDelete       = false;
    bool    m_isNoClip                = false;
    bool    m_isUpdateMapCountingDown = false;
    float   m_glowIntensity           = 0.f;
    float   m_gameOverCountDown       = 3.f;
    float   m_updateMapCountDown      = 1.f;
    bool    m_glowIncreasing          = false;
    Vec2    m_baseCameraPos           = Vec2::ZERO;

    std::vector<Map*> m_maps;
    Map*              m_currentMap      = nullptr;
    SpriteSheet*      m_tileSpriteSheet = nullptr;
    PlayerTank*       m_playerTank      = nullptr;

    SoundID         m_attractModeBgm       = 0;
    SoundPlaybackID m_attractModePlayback  = 0;
    SoundID         m_InGameBgm            = 0;
    SoundPlaybackID m_InGamePlayback       = 0;
    SoundID         m_gameWinBgm           = 0;
    SoundPlaybackID m_gameWinPlayback      = 0;
    SoundID         m_gameLoseBgm          = 0;
    SoundPlaybackID m_gameLosePlayback     = 0;
    SoundID         m_clickSound           = 0;
    SoundID         m_pauseSound           = 0;
    SoundID         m_resumeSound          = 0;
    SoundID         m_playerTankShootSound = 0;
    SoundID         m_playerTankHitSound   = 0;
    SoundID         m_enemyDiedSound       = 0;
    SoundID         m_enemyHitSound        = 0;
    SoundID         m_enemyShootSound      = 0;
    SoundID         m_exitMapSound         = 0;
    SoundID         m_bulletBounceSound    = 0;
    SoundID         m_enemyDiscoverSound   = 0;
    float           m_InGameBgmSpeed       = 1.f;

    // float numTilesInViewHorizontally
};
