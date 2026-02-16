//----------------------------------------------------------------------------------------------------
// Entity.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
class Map;
class Entity;
class Texture;
class TileHeatMap;
typedef std::vector<Entity*> EntityList;

//----------------------------------------------------------------------------------------------------
enum EntityType: int
{
    ENTITY_TYPE_UNKNOWN = -1,
    ENTITY_TYPE_PLAYER_TANK,
    ENTITY_TYPE_SCORPIO,
    ENTITY_TYPE_LEO,
    ENTITY_TYPE_ARIES,
    ENTITY_TYPE_BULLET,
    ENTITY_TYPE_EXPLOSION,
    ENTITY_TYPE_DEBRIS,
    NUM_ENTITY_TYPES
};

//----------------------------------------------------------------------------------------------------
enum EntityFaction: int
{
    ENTITY_FACTION_UNKNOWN = -1,
    ENTITY_FACTION_GOOD,
    ENTITY_FACTION_NEUTRAL,
    ENTITY_FACTION_EVIL,
    NUM_ENTITY_FACTIONS
};

//-----------------------------------------------------------------------------------------------
class Entity
{
    friend class Map;

public:
    Entity(Map* map, EntityType type, EntityFaction faction);
    virtual ~Entity() = default; //add an addition secrete pointer to the class

    virtual void Update(float deltaSeconds) = 0;
    virtual void Render() const = 0;
    virtual void DebugRender() const = 0;
    virtual void TurnToward(float& orientationDegrees, float targetOrientationDegrees, float deltaSeconds, float rotationSpeed);
    void         MoveToward(Vec2& currentPosition, Vec2 const& targetPosition, float moveSpeed, float deltaSeconds);
    void         WanderAround(float deltaSeconds, float moveSpeed, float rotateSpeed);
    void         UpdateBehavior(float deltaSeconds, bool isChasing);
    void         RenderHealthBar() const;

// TODO: MAKE THIS
// virtual  void TurnTowardPosition(Vec2 const& targetPos, float maxTurnDegrees); 

    Map*              m_map                     = nullptr;
    EntityType        m_type                    = ENTITY_TYPE_UNKNOWN;
    EntityFaction     m_faction                 = ENTITY_FACTION_UNKNOWN;
    Vec2              m_position                = Vec2::ZERO;
    Vec2              m_velocity                = Vec2::ZERO;
    // Vec2              m_targetLastKnownPosition = Vec2::ZERO;
    // Vec2              m_nextWayPosition         = Vec2::ZERO;
    Vec2              m_goalPosition            = Vec2::ZERO;
    std::vector<Vec2> m_pathPoints;
    TileHeatMap*      m_heatMap = nullptr;
    AABB2             m_bodyBounds = AABB2::NEG_HALF_TO_HALF;
    Texture const*    m_bodyTexture              = nullptr;
    float             m_moveSpeed                = 0.f;
    float             m_rotateSpeed              = 0.f;
    float             m_orientationDegrees       = 0.f;
    float             m_targetOrientationDegrees = 0.f;
    float             m_detectRange              = 0.f;
    float             m_physicsRadius            = 0.f;
    float             m_timeSinceLastRoll        = 0.f;
    int               m_health                   = 0;
    int               m_totalHealth              = 0;
    bool              m_isDead                   = false;
    bool              m_isGarbage                = false;
    bool              m_isPushedByEntities       = false;
    bool              m_doesPushEntities         = false;
    bool              m_isPushedByWalls          = false;
    bool              m_canSwim                  = false;
    bool              m_hasTarget                = false;
    bool              m_isChasing                = false;
    bool              m_hasPlayedDiscoverSound   = false;
};
