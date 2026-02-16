//----------------------------------------------------------------------------------------------------
// Leo.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Leo.hpp"

#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"

//----------------------------------------------------------------------------------------------------
Leo::Leo(Map* map, EntityType const type, EntityFaction const faction)
    : Entity(map, type, faction)
{
    m_physicsRadius      = g_gameConfigBlackboard.GetValue("leoPhysicsRadius", 0.25f);
    m_detectRange        = g_gameConfigBlackboard.GetValue("leoDetectRange", 10.f);
    m_moveSpeed          = g_gameConfigBlackboard.GetValue("leoMoveSpeed", 0.5f);
    m_rotateSpeed        = g_gameConfigBlackboard.GetValue("leoRotateSpeed", 90.f);
    m_health             = g_gameConfigBlackboard.GetValue("leoInitHealth", 3);
    m_isPushedByWalls    = g_gameConfigBlackboard.GetValue("leoIsPushedByWalls", true);
    m_isPushedByEntities = g_gameConfigBlackboard.GetValue("leoIsPushedByEntities", true);
    m_doesPushEntities   = g_gameConfigBlackboard.GetValue("leoDoesPushEntities", true);
    m_canSwim            = g_gameConfigBlackboard.GetValue("leoCanSwim", false);

    m_totalHealth = m_health;

    m_bodyTexture = g_resourceSubsystem->CreateOrGetTextureFromFile(LEO_BODY_IMG);
}

//----------------------------------------------------------------------------------------------------
void Leo::Update(float const deltaSeconds)
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
void Leo::Render() const
{
    if (m_isDead)
        return;

    RenderBody();
    RenderHealthBar();
}

//----------------------------------------------------------------------------------------------------
void Leo::DebugRender() const
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
                  m_position + fwdNormal,
                  0.025f,
                  Rgba8::YELLOW);
}

//----------------------------------------------------------------------------------------------------
void Leo::UpdateBody(float const deltaSeconds)
{
    m_timeSinceLastRoll += deltaSeconds;

    UpdateShootCoolDown(deltaSeconds);

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

        if (degreesToTarget < m_shootDegreesThreshold &&
            m_shootCoolDown <= 0.0f)
        {
            m_map->SpawnNewEntity(ENTITY_TYPE_BULLET, ENTITY_FACTION_EVIL, m_position, m_orientationDegrees);
            m_shootCoolDown = g_gameConfigBlackboard.GetValue("leoShootCoolDown", 1.f);
            g_audio->StartSound(g_game->GetEnemyShootSoundID());
        }
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
void Leo::RenderBody() const
{
    VertexList_PCU bodyVerts;
    AddVertsForAABB2D(bodyVerts, m_bodyBounds, Rgba8::WHITE);

    TransformVertexArrayXY3D(static_cast<int>(bodyVerts.size()), bodyVerts.data(),
                             1.0f, m_orientationDegrees, m_position);

    g_renderer->BindTexture(m_bodyTexture);
    g_renderer->DrawVertexArray(static_cast<int>(bodyVerts.size()), bodyVerts.data());

    VertexList_PCU   stateVerts;
    String const stateStr   = m_hasTarget ? "Chase" : "Wander";
    Rgba8 const  stateColor = m_hasTarget ? Rgba8::RED : Rgba8::WHITE;

    BitmapFont*    bitmapFont = g_resourceSubsystem->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont");
    bitmapFont->AddVertsForTextInBox2D(stateVerts, stateStr, m_bodyBounds, 1.f, stateColor);
    TransformVertexArrayXY3D(static_cast<int>(stateVerts.size()), stateVerts.data(),
                             1.0f, 0, m_position);
    g_renderer->BindTexture(&bitmapFont->GetTexture());
    g_renderer->DrawVertexArray(static_cast<int>(stateVerts.size()), stateVerts.data());
}

//----------------------------------------------------------------------------------------------------
void Leo::UpdateShootCoolDown(float const deltaSeconds)
{
    if (m_shootCoolDown > 0.0f)
    {
        m_shootCoolDown -= deltaSeconds;
    }
}
