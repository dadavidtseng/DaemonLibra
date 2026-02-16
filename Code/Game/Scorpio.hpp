//----------------------------------------------------------------------------------------------------
// Scorpio.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Game/Entity.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
class Texture;

//----------------------------------------------------------------------------------------------------
class Scorpio : public Entity
{
public:
    Scorpio(Map* map, EntityType type, EntityFaction faction);

    void Update(float deltaSeconds) override;
    void Render() const override;
    void DebugRender() const override;

private:
    void UpdateTurret(float deltaSeconds);
    void RenderBody() const;
    void RenderTurret() const;
    void RenderLaser() const;

    AABB2    m_turretBounds             = AABB2::NEG_HALF_TO_HALF;
    Texture* m_turretTexture            = nullptr;
    float    m_turretOrientationDegrees = 0.f;
    float    m_shootCoolDown            = 0.f;
    float    m_turretRotateSpeed        = g_gameConfigBlackboard.GetValue("scorpioTurretRotateSpeed", 90.f);
    float    m_shootDegreesThreshold    = g_gameConfigBlackboard.GetValue("scorpioShootDegreesThreshold", 5.f);
};
