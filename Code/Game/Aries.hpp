//----------------------------------------------------------------------------------------------------
// Aries.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Entity.hpp"

//----------------------------------------------------------------------------------------------------
class Aries : public Entity
{
public:
    Aries(Map* map, EntityType type, EntityFaction faction);

    void Update(float deltaSeconds) override;
    void Render() const override;
    void DebugRender() const override;

private:
    void UpdateBody(float deltaSeconds);
    void RenderBody() const;
};
