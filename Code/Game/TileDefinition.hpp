//----------------------------------------------------------------------------------------------------
// TileDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//----------------------------------------------------------------------------------------------------
struct TileDefinition
{
    TileDefinition(XmlElement const& tileDefElement, SpriteSheet const& spriteSheet);
    ~TileDefinition();

    static void                         InitializeTileDefs(SpriteSheet const& spriteSheet);
    static TileDefinition const*        GetTileDefByName(String const& name);
    static StringList                   GetTileNames();
    static std::vector<TileDefinition*> s_tileDefinitions;

    String           GetName() const { return m_name; }
    SpriteDefinition GetSpriteDef() const { return m_spriteDef; }
    bool             IsSolid() const { return m_isSolid; }
    bool             IsWater() const { return m_isWater; }
    Rgba8            GetTintColor() const { return m_tintColor; }
    AABB2            GetUVs() const { return m_spriteDef.GetUVs(); }

private:
    String           m_name;
    SpriteDefinition m_spriteDef;
    bool             m_isSolid = false;
    bool             m_isWater = false;
    Rgba8            m_tintColor;
};
