//----------------------------------------------------------------------------------------------------
// MapDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/MapDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
std::vector<MapDefinition*> MapDefinition::s_mapDefinitions;

//----------------------------------------------------------------------------------------------------
MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
    m_name                   = ParseXmlAttribute(mapDefElement, "name", "Unnamed");
    m_index                  = ParseXmlAttribute(mapDefElement, "index", -1);
    m_worm01TileName         = ParseXmlAttribute(mapDefElement, "worm01TileName", "Unnamed");
    m_worm02TileName         = ParseXmlAttribute(mapDefElement, "worm02TileName", "Unnamed");
    m_worm03TileName         = ParseXmlAttribute(mapDefElement, "worm03TileName", "Unnamed");
    m_worm01Num              = ParseXmlAttribute(mapDefElement, "worm01Num", -1);
    m_worm02Num              = ParseXmlAttribute(mapDefElement, "worm02Num", -1);
    m_worm03Num              = ParseXmlAttribute(mapDefElement, "worm03Num", -1);
    m_worm01Length           = ParseXmlAttribute(mapDefElement, "worm01Length", -1);
    m_worm02Length           = ParseXmlAttribute(mapDefElement, "worm02Length", -1);
    m_worm03Length           = ParseXmlAttribute(mapDefElement, "worm03Length", -1);
    m_scorpioSpawnPercentage = ParseXmlAttribute(mapDefElement, "scorpioSpawnPercentage", -1.f);
    m_leoSpawnPercentage     = ParseXmlAttribute(mapDefElement, "leoSpawnPercentage", -1.f);
    m_ariesSpawnPercentage   = ParseXmlAttribute(mapDefElement, "ariesSpawnPercentage", -1.f);
    m_dimensions             = ParseXmlAttribute(mapDefElement, "dimensions", IntVec2(-1, -1));
}

//----------------------------------------------------------------------------------------------------
MapDefinition::~MapDefinition()
{
    for (MapDefinition const* mapDef : s_mapDefinitions)
    {
        delete mapDef;
    }

    s_mapDefinitions.clear();
}

//----------------------------------------------------------------------------------------------------
STATIC void MapDefinition::InitializeMapDefs()
{
    XmlDocument mapDefXml;
    if (mapDefXml.LoadFile("Data/Definitions/MapDefinitions.xml") != XmlResult::XML_SUCCESS)
    {
        return;
    }

    if (XmlElement* root = mapDefXml.FirstChildElement("MapDefinitions"))
    {
        for (XmlElement* element = root->FirstChildElement("MapDefinition"); element != nullptr; element = element->NextSiblingElement("MapDefinition"))
        {
            MapDefinition* mapDef = new MapDefinition(*element);
            s_mapDefinitions.push_back(mapDef);
        }
    }
}

//----------------------------------------------------------------------------------------------------
STATIC MapDefinition const* MapDefinition::GetTileDefByName(String const& name)
{
    for (MapDefinition const* mapDef : s_mapDefinitions)
    {
        if (mapDef->m_name == name)
        {
            return mapDef;
        }
    }

    return nullptr;
}
