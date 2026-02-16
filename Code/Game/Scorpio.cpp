//----------------------------------------------------------------------------------------------------
// Scorpio.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Scorpio.hpp"

#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"

//----------------------------------------------------------------------------------------------------
Scorpio::Scorpio(Map* map, EntityType const type, EntityFaction const faction)
    : Entity(map, type, faction)
{
    m_physicsRadius      = g_gameConfigBlackboard.GetValue("scorpioPhysicsRadius", 0.f);
    m_detectRange        = g_gameConfigBlackboard.GetValue("scorpioDetectRange", 0.f);
    m_isPushedByWalls    = g_gameConfigBlackboard.GetValue("scorpioIsPushedByWalls", -1);
    m_isPushedByEntities = g_gameConfigBlackboard.GetValue("scorpioIsPushedByEntities", -1);
    m_doesPushEntities   = g_gameConfigBlackboard.GetValue("scorpioDoesPushEntities", -1);
    m_canSwim            = g_gameConfigBlackboard.GetValue("scorpioCanSwim", -1);
    m_goalPosition       = m_position;
    m_health             = g_gameConfigBlackboard.GetValue("scorpioInitHealth", 5);

    m_totalHealth = m_health;
    
    m_bodyTexture   = g_resourceSubsystem->CreateOrGetTextureFromFile(SCORPIO_BODY_IMG);
    m_turretTexture = g_resourceSubsystem->CreateOrGetTextureFromFile(SCORPIO_TURRET_IMG);
}

//----------------------------------------------------------------------------------------------------
void Scorpio::Update(float const deltaSeconds)
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

    UpdateTurret(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void Scorpio::Render() const
{
    if (m_isDead)
        return;

    RenderBody();
    RenderTurret();
    RenderLaser();
    RenderHealthBar();
}

//----------------------------------------------------------------------------------------------------
void Scorpio::DebugRender() const
{
    if (m_isDead)
        return;

    Vec2 const fwdNormal  = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees);
    Vec2 const leftNormal = fwdNormal.GetRotated90Degrees();

    DebugDrawRing(m_position,
                  m_physicsRadius,
                  0.03f,
                  Rgba8::CYAN);

    DebugDrawLine(m_position,
                  m_position + fwdNormal,
                  0.03f,
                  Rgba8::RED);
    DebugDrawLine(m_position,
                  m_position + leftNormal,
                  0.03f,
                  Rgba8::GREEN);
}

//----------------------------------------------------------------------------------------------------
void Scorpio::UpdateTurret(float const deltaSeconds)
{
    if (m_shootCoolDown > 0.0f)
    {
        m_shootCoolDown -= deltaSeconds;
    }

    // Turn and shoot ( or turn idly)
    PlayerTank const* playerTank = g_game->GetPlayerTank();
    if (m_map->HasLineOfSight(m_position, playerTank->m_position, m_detectRange) && !playerTank->m_isDead)
    {
        // Turn toward player
        float const targetOrientationDegrees = (m_goalPosition - m_position).GetOrientationDegrees();

        TurnToward(m_turretOrientationDegrees, targetOrientationDegrees, deltaSeconds, m_turretRotateSpeed);

        // Shot at player if facing close enough to orientation
        Vec2 const  dispToTarget    = playerTank->m_position - m_position;
        Vec2 const  myFwdNormal     = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees);
        float const degreesToTarget = GetAngleDegreesBetweenVectors2D(dispToTarget, myFwdNormal);

        if (degreesToTarget < m_shootDegreesThreshold &&
            m_shootCoolDown <= 0.0f)
        {
            m_map->SpawnNewEntity(ENTITY_TYPE_BULLET, ENTITY_FACTION_EVIL, m_position + myFwdNormal * 0.45f, m_turretOrientationDegrees);
            m_shootCoolDown = g_gameConfigBlackboard.GetValue("scorpioShootCoolDown", 0.3f);
            g_audio->StartSound(g_game->GetEnemyShootSoundID());
            m_map->SpawnNewEntity(ENTITY_TYPE_EXPLOSION, ENTITY_FACTION_NEUTRAL, m_position, m_orientationDegrees);
        }

        m_goalPosition = playerTank->m_position;
    }
    else
    {
        // turn blindly
        m_turretOrientationDegrees += deltaSeconds * m_turretRotateSpeed;
    }
}

//----------------------------------------------------------------------------------------------------
void Scorpio::RenderBody() const
{
    std::vector<Vertex_PCU> bodyVerts;
    AddVertsForAABB2D(bodyVerts, m_bodyBounds, Rgba8(255, 255, 255));

    TransformVertexArrayXY3D(static_cast<int>(bodyVerts.size()), bodyVerts.data(),
                             1.0f, m_orientationDegrees, m_position);

    g_renderer->BindTexture(m_bodyTexture);
    g_renderer->DrawVertexArray(static_cast<int>(bodyVerts.size()), bodyVerts.data());
}

//----------------------------------------------------------------------------------------------------
void Scorpio::RenderTurret() const
{
    std::vector<Vertex_PCU> turretVerts;
    AddVertsForAABB2D(turretVerts, m_turretBounds, Rgba8(255, 255, 255));

    TransformVertexArrayXY3D(static_cast<int>(turretVerts.size()), turretVerts.data(),
                             1.0f, m_orientationDegrees + m_turretOrientationDegrees, m_position);

    g_renderer->BindTexture(m_turretTexture);
    g_renderer->DrawVertexArray(static_cast<int>(turretVerts.size()), turretVerts.data());
}

//----------------------------------------------------------------------------------------------------
void Scorpio::RenderLaser() const
{
    Vec2 const            fwdNormal       = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees);
    Ray2 const            ray             = Ray2(m_position, fwdNormal.GetNormalized(), 10000);
    RaycastResult2D const raycastResult2D = m_map->RaycastVsTiles(ray);

    DebugDrawLine(m_position + fwdNormal * 0.45f, raycastResult2D.m_impactPosition, 0.05f, Rgba8::RED);
}
