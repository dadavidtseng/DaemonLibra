//----------------------------------------------------------------------------------------------------
// GameCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
struct Vec2;
struct Rgba8;
class App;
class AudioSystem;
class BitmapFont;
class Game;
class InputSystem;
class Renderer;
class RandomNumberGenerator;
class Window;

// one-time declaration
extern App*                   g_theApp;
extern AudioSystem*           g_theAudio;
extern BitmapFont*            g_theBitmapFont;
extern Game*                  g_theGame;
extern Renderer*              g_theRenderer;
extern RandomNumberGenerator* g_theRNG;
extern Window*                g_theWindow;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void DebugDrawGlowCircle(Vec2 const& center, float radius, Rgba8 const& color, float glowIntensity);
void DebugDrawGlowBox(Vec2 const& center, Vec2 const& dimensions, Rgba8 const& color, float glowIntensity);
void DebugDrawBoxRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

//-----------------------------------------------------------------------------------------------
// Audio-related
//
// extern const char* ATTRACT_MODE_BGM;
extern const char* IN_GAME_BGM;
extern const char* GAME_WIN_BGM;
extern const char* GAME_LOSE_BGM;
extern const char* CLICK_SOUND;
extern const char* PAUSE_SOUND;
extern const char* RESUME_SOUND;
extern const char* PLAYER_TANK_SHOOT_SOUND;
extern const char* PLAYER_TANK_HIT_SOUND;
extern const char* ENEMY_DIED_SOUND;
extern const char* ENEMY_HIT_SOUND;
extern const char* ENEMY_SHOOT_SOUND;
extern const char* EXIT_MAP_SOUND;
extern const char* BULLET_BOUNCE_SOUND;
extern const char* ENEMY_DISCOVER_SOUND;

//-----------------------------------------------------------------------------------------------
// Texture-related
//
extern const char* PLAYER_TANK_BODY_IMG;
extern const char* PLAYER_TANK_TURRET_IMG;
extern const char* SCORPIO_BODY_IMG;
extern const char* SCORPIO_TURRET_IMG;
extern const char* LEO_BODY_IMG;
extern const char* ARIES_BODY_IMG;
extern const char* BULLET_GOOD_IMG;
extern const char* BULLET_EVIL_IMG;
extern const char* TILE_TEXTURE_IMG;
