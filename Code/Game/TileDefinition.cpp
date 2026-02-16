//----------------------------------------------------------------------------------------------------
// TileDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/TileDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class SpriteSheet;

//----------------------------------------------------------------------------------------------------
std::vector<TileDefinition*> TileDefinition::s_tileDefinitions;

//----------------------------------------------------------------------------------------------------
TileDefinition::TileDefinition(XmlElement const& tileDefElement, SpriteSheet const& spriteSheet)
{
    m_name                     = ParseXmlAttribute(tileDefElement, "name", "Unnamed");
    m_isSolid                  = ParseXmlAttribute(tileDefElement, "isSolid", false);
    m_isWater                  = ParseXmlAttribute(tileDefElement, "isWater", false);
    IntVec2 const spriteCoords = ParseXmlAttribute(tileDefElement, "spriteCoords", IntVec2(-1, -1));
    int const     spriteIndex  = spriteCoords.x + spriteCoords.y * 8;

    if (spriteIndex != -1)
    {
        m_spriteDef = spriteSheet.GetSpriteDef(spriteIndex);
    }

    m_tintColor = ParseXmlAttribute(tileDefElement, "tintColor", Rgba8::WHITE);
}

//----------------------------------------------------------------------------------------------------
TileDefinition::~TileDefinition()
{
    for (TileDefinition const* tileDef : s_tileDefinitions)
    {
        delete tileDef;
    }

    s_tileDefinitions.clear();
}

//----------------------------------------------------------------------------------------------------
STATIC void TileDefinition::InitializeTileDefs(SpriteSheet const& spriteSheet)
{
    XmlDocument tileDefXml;

    if (tileDefXml.LoadFile("Data/Definitions/TileDefinitions.xml") != XmlResult::XML_SUCCESS)
        return;

    if (XmlElement* root = tileDefXml.FirstChildElement("TileDefinitions"))
    {
        for (XmlElement* element = root->FirstChildElement("TileDefinition"); element != nullptr; element = element->NextSiblingElement("TileDefinition"))
        {
            TileDefinition* tileDef = new TileDefinition(*element, spriteSheet);
            s_tileDefinitions.push_back(tileDef);
        }
    }
}

//----------------------------------------------------------------------------------------------------
STATIC TileDefinition const* TileDefinition::GetTileDefByName(String const& name)
{
    for (TileDefinition const* tileDef : s_tileDefinitions)
    {
        if (tileDef->GetName() == name)
        {
            return tileDef;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
STATIC StringList TileDefinition::GetTileNames()
{
    StringList tileNames;

    for (TileDefinition const* tileDef : s_tileDefinitions)
    {
        if (tileDef)
        {
            tileNames.push_back(tileDef->GetName());
        }
    }

    return tileNames;
}
