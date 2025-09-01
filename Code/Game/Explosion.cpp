//----------------------------------------------------------------------------------------------------
// Explosion.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Explosion.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"

//----------------------------------------------------------------------------------------------------
Explosion::Explosion(Map* map, EntityType const type, EntityFaction const faction)
    : Entity(map, type, faction)
{
    m_isPushedByWalls    = g_gameConfigBlackboard.GetValue("explosionIsPushedByWalls", true);
    m_isPushedByEntities = g_gameConfigBlackboard.GetValue("explosionIsPushedByEntities", true);
    m_doesPushEntities   = g_gameConfigBlackboard.GetValue("explosionDoesPushEntities", true);

    m_health                          = g_gameConfigBlackboard.GetValue("explosionInitHealth", 1);
    Texture const* const tileTexture  = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");
    IntVec2 const        spriteCoords = IntVec2(5, 5);
    m_spriteSheet                     = new SpriteSheet(*tileTexture, spriteCoords);

    m_bodyBounds    = AABB2(Vec2(-0.5f, -0.5f), Vec2(0.5f, 0.5f));
}

//----------------------------------------------------------------------------------------------------
void Explosion::Update(float const deltaSeconds)
{
    if (m_isDead)
        return;

    m_animationTime += deltaSeconds;

    if (m_animationTime >= 2.f)
        m_health = 0;

    if (m_health <= 0)
    {
        m_isGarbage = true;
        m_isDead    = true;
    }
}

//----------------------------------------------------------------------------------------------------
void Explosion::Render() const
{
    if (m_isDead)
        return;

    RenderBody();
}

//----------------------------------------------------------------------------------------------------
void Explosion::DebugRender() const
{
    if (m_isDead)
        return;

    DebugDrawRing(m_position,
                  m_physicsRadius,
                  0.03f,
                  Rgba8::CYAN);
}

//----------------------------------------------------------------------------------------------------
void Explosion::RenderBody() const
{
    VertexList_PCU vertexArray;

    SpriteAnimDefinition const myAnim(*m_spriteSheet, 0, 24, 10.f, SpriteAnimPlaybackType::ONCE);

    SpriteDefinition const& spriteDef = myAnim.GetSpriteDefAtTime(m_animationTime);

    Vec2 const uvMins = spriteDef.GetUVsMins();
    Vec2 const uvMaxs = spriteDef.GetUVsMaxs();

    AddVertsForAABB2D(vertexArray, m_bodyBounds, Rgba8::WHITE, uvMins, uvMaxs);

    TransformVertexArrayXY3D(static_cast<int>(vertexArray.size()), vertexArray.data(),
                             1.f, 0.f, m_position);

    g_theRenderer->BindTexture(&spriteDef.GetTexture());
    g_theRenderer->SetBlendMode(eBlendMode::ADDITIVE);
    g_theRenderer->DrawVertexArray(static_cast<int>(vertexArray.size()), vertexArray.data());
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
}
