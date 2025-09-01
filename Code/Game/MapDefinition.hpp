//----------------------------------------------------------------------------------------------------
// MapDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//----------------------------------------------------------------------------------------------------
struct MapDefinition
{
    explicit MapDefinition(XmlElement const& mapDefElement);
    ~MapDefinition();

    static void                        InitializeMapDefs();
    static MapDefinition const*        GetTileDefByName(String const& name);
    static std::vector<MapDefinition*> s_mapDefinitions;

    String        GetName() const { return m_name; }
    int           GetIndex() const { return m_index; }
    String const& GetWorm01TileName() const { return m_worm01TileName; }
    String const& GetWorm02TileName() const { return m_worm02TileName; }
    String const& GetWorm03TileName() const { return m_worm03TileName; }
    int           GetWorm01Num() const { return m_worm01Num; }
    int           GetWorm02Num() const { return m_worm02Num; }
    int           GetWorm03Num() const { return m_worm03Num; }
    int           GetWorm01Length() const { return m_worm01Length; }
    int           GetWorm02Length() const { return m_worm02Length; }
    int           GetWorm03Length() const { return m_worm03Length; }
    float         GetScorpioSpawnPercentage() const { return m_scorpioSpawnPercentage; }
    float         GetLeoSpawnPercentage() const { return m_leoSpawnPercentage; }
    float         GetAriesSpawnPercentage() const { return m_ariesSpawnPercentage; }
    IntVec2       GetDimensions() const { return m_dimensions; }

private:
    String  m_name;
    int     m_index = 0;
    String  m_worm01TileName;
    String  m_worm02TileName;
    String  m_worm03TileName;
    int     m_worm01Num              = 0;
    int     m_worm02Num              = 0;
    int     m_worm03Num              = 0;
    int     m_worm01Length           = 0;
    int     m_worm02Length           = 0;
    int     m_worm03Length           = 0;
    float   m_scorpioSpawnPercentage = 0.f;
    float   m_leoSpawnPercentage     = 0.f;
    float   m_ariesSpawnPercentage   = 0.f;
    IntVec2 m_dimensions             = IntVec2::ZERO;
};
