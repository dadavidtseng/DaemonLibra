//----------------------------------------------------------------------------------------------------
// PlayerTank.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/Entity.hpp"

//----------------------------------------------------------------------------------------------------
class Texture;

//----------------------------------------------------------------------------------------------------
class PlayerTank : public Entity
{
public:
    static bool SHOOT(EventArgs& args);
    PlayerTank(Map* map, EntityType type, EntityFaction faction);

    void Update(float deltaSeconds) override;
    void Render() const override;
    void DebugRender() const override;
    void SetBodyScale(float scale) { m_bodyScale = scale; }

private:
    void UpdateBody(float deltaSeconds);
    void UpdateTurret(float deltaSeconds);
    void RenderBody() const;
    void RenderTurret() const;

    AABB2    m_turretBounds;
    Texture* m_turretTexture                = nullptr;
    float    m_turretRelativeOrientation    = 0.f;
    float    m_turretGoalOrientationDegrees = 0.f;
    float    m_turretRotateSpeed            = g_gameConfigBlackboard.GetValue("playerTankTurretRotateSpeed", 360.f);
    float    m_shootCoolDown                = 0.f;
    float    m_bodyScale                    = 0.f;
    bool     m_isExiting                    = false;
    bool     m_isEntering                   = false;
    Vec2     m_bodyInput                    = Vec2::ZERO;
};
