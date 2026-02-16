//----------------------------------------------------------------------------------------------------
// Aries.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Aries.hpp"

#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"

//----------------------------------------------------------------------------------------------------
Aries::Aries(Map* map, EntityType const type, EntityFaction const faction)
    : Entity(map, type, faction)
{
    m_physicsRadius      = g_gameConfigBlackboard.GetValue("ariesPhysicsRadius", 0.25f);
    m_detectRange        = g_gameConfigBlackboard.GetValue("ariesDetectRange", 10.f);
    m_moveSpeed          = g_gameConfigBlackboard.GetValue("ariesMoveSpeed", 0.5f);
    m_rotateSpeed        = g_gameConfigBlackboard.GetValue("ariesRotateSpeed", 90.f);
    m_health             = g_gameConfigBlackboard.GetValue("ariesInitHealth", 8);
    m_isPushedByWalls    = g_gameConfigBlackboard.GetValue("ariesIsPushedByWalls", true);
    m_isPushedByEntities = g_gameConfigBlackboard.GetValue("ariesIsPushedByEntities", true);
    m_doesPushEntities   = g_gameConfigBlackboard.GetValue("ariesDoesPushEntities", true);
    m_canSwim            = g_gameConfigBlackboard.GetValue("ariesCanSwim", false);

    m_totalHealth = m_health;
    
    m_bodyTexture = g_resourceSubsystem->CreateOrGetTextureFromFile(ARIES_BODY_IMG);
}

//----------------------------------------------------------------------------------------------------
void Aries::Update(const float deltaSeconds)
{
    if (m_isDead)
        return;

    if (g_game->GetPlayerTank()->m_isDead)
        return;

    if (m_health <= 0)
    {
        g_audio->StartSound(g_game->GetEnemyDiedSoundID());
        m_map->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, ENTITY_FACTION_NEUTRAL, m_position, m_orientationDegrees);
        m_isGarbage = true;
        m_isDead    = true;
    }

    UpdateBody(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void Aries::Render() const
{
    if (m_isDead)
        return;

    RenderBody();
    RenderHealthBar();
}

//----------------------------------------------------------------------------------------------------
void Aries::DebugRender() const
{
    if (m_isDead)
        return;

    Vec2 const fwdNormal  = Vec2::MakeFromPolarDegrees(m_orientationDegrees);
    Vec2 const leftNormal = fwdNormal.GetRotated90Degrees();

    DebugDrawRing(m_position,
                  m_physicsRadius,
                  0.05f,
                  Rgba8::CYAN);

    DebugDrawLine(m_position,
                  m_position + fwdNormal,
                  0.05f,
                  Rgba8::RED);
    DebugDrawLine(m_position,
                  m_position + leftNormal,
                  0.05f,
                  Rgba8::GREEN);

    DebugDrawLine(m_position,
                  m_goalPosition,
                  0.05f,
                  Rgba8::GREY);

    DebugDrawGlowCircle(m_goalPosition,
                        0.1f,
                        Rgba8::GREY,
                        1.f);

    DebugDrawLine(m_position,
                  m_pathPoints.back(),
                  0.05f,
                  Rgba8::WHITE);

    DebugDrawGlowCircle(m_pathPoints.back(),
                        0.1f,
                        Rgba8::WHITE,
                        1.f);


    DebugDrawLine(m_position,
                  m_position + m_velocity,
                  0.025f,
                  Rgba8::YELLOW);
}

//----------------------------------------------------------------------------------------------------
void Aries::UpdateBody(const float deltaSeconds)
{
    m_timeSinceLastRoll += deltaSeconds;

    PlayerTank const* playerTank = g_game->GetPlayerTank();

    if (!playerTank)
        return;

    Vec2 const  dispToTarget    = m_goalPosition - m_position;
    Vec2 const  fwdNormal       = Vec2::MakeFromPolarDegrees(m_orientationDegrees);
    float const degreesToTarget = GetAngleDegreesBetweenVectors2D(dispToTarget, fwdNormal);

    if (degreesToTarget < 45.f &&
        m_hasTarget)
    {
        m_targetOrientationDegrees = Atan2Degrees(dispToTarget.y, dispToTarget.x);

        TurnToward(m_orientationDegrees,
                   m_targetOrientationDegrees,
                   deltaSeconds,
                   m_rotateSpeed);

        // m_velocity = Vec2::MakeFromPolarDegrees(m_orientationDegrees) * m_moveSpeed * deltaSeconds;
        // m_position += m_velocity;

    }

    // TurnToward if entity sees target
    if (m_map->HasLineOfSight(m_position, playerTank->m_position, m_detectRange))
    {
        m_hasTarget = true;

        UpdateBehavior(deltaSeconds, true);
    }
    else
    {
        // if not, wander around
        UpdateBehavior(deltaSeconds, false);
    }
}

//----------------------------------------------------------------------------------------------------
void Aries::RenderBody() const
{
    std::vector<Vertex_PCU> bodyVerts;
    AddVertsForAABB2D(bodyVerts, m_bodyBounds, Rgba8::WHITE);

    TransformVertexArrayXY3D(static_cast<int>(bodyVerts.size()), bodyVerts.data(),
                             1.0f, m_orientationDegrees, m_position);

    g_renderer->BindTexture(m_bodyTexture);
    g_renderer->DrawVertexArray(static_cast<int>(bodyVerts.size()), bodyVerts.data());
}
