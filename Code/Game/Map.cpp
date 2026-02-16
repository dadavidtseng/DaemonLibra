//----------------------------------------------------------------------------------------------------
// Map.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Map.hpp"

#include <cmath>
#include <queue>

#include "Debris.hpp"
#include "Explosion.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Game/Aries.hpp"
#include "Game/Bullet.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Leo.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Scorpio.hpp"
#include "Game/Tile.hpp"

//----------------------------------------------------------------------------------------------------
Map::Map(MapDefinition const& mapDef)
    : m_mapDef(&mapDef)
{
    m_dimensions = mapDef.GetDimensions();
    m_tiles.reserve(static_cast<size_t>(m_dimensions.x) * static_cast<size_t>(m_dimensions.y));
    m_startPosition = IntVec2::ONE;
    m_exitPosition  = IntVec2(m_dimensions.x - 2, m_dimensions.y - 2);

    InitializeTileHeatMaps();
    GenerateAllTiles();
    SpawnNewNPCs();
    // GenerateHeatMaps(*m_tileHeatMaps[0]);
    // GenerateHeatMaps(*m_tileHeatMaps[1]);
    // GenerateHeatMaps(*m_tileHeatMaps[2]);
    // GenerateHeatMaps(*m_tileHeatMaps[3]);
    PopulateDistanceField(*m_tileHeatMaps[0], m_startPosition, 999.f);
    PopulateDistanceFieldForLandBased(*m_tileHeatMaps[1]);
    PopulateDistanceFieldForAmphibian(*m_tileHeatMaps[2]);
    PopulateDistanceFieldForEntity(*m_tileHeatMaps[3], m_startPosition, 999.f);
}

//----------------------------------------------------------------------------------------------------
Map::~Map()
{
    m_allEntities.clear();
    m_entitiesByType->clear();
    m_agentsByFaction->clear();
    m_bulletsByFaction->clear();
    m_tiles.clear();
    m_tileHeatMaps.clear();

    delete m_currentSelectedEntity;
    m_currentSelectedEntity = nullptr;
}

//----------------------------------------------------------------------------------------------------
void Map::Update(float const deltaSeconds)
{
    if (g_theGame->IsAttractMode()) return;

    if (g_input->WasKeyJustPressed(KEYCODE_F6))
    {
        // Increment the index
        m_currentTileHeatMapIndex++;

        // Cycle between -1 and 3
        if (m_currentTileHeatMapIndex > 3)
        {
            m_currentTileHeatMapIndex = -1;
        }

        if (m_currentTileHeatMapIndex == 3)
        {
            for (Entity* entity : m_allEntities)
            {
                if (entity->m_type == ENTITY_TYPE_LEO)
                {
                    m_currentSelectedEntity = entity;
                    break;
                }
            }
        }
        // else if (m_currentTileHeatMapIndex >= 0)
        // {
        //     m_currentTileHeatMap = m_tileHeatMaps[m_currentTileHeatMapIndex];
        // }
        // else
        // {
        //     delete m_currentTileHeatMap;
        //     m_currentTileHeatMap = nullptr;
        // }
    }


    UpdateEntities(deltaSeconds);
    PushEntitiesOutOfEachOther(m_allEntities, m_allEntities);
    CheckEntityVsEntityCollision(m_entitiesByType[ENTITY_TYPE_BULLET], m_allEntities);
    PushEntitiesOutOfWalls();
    DeleteGarbageEntities();
}

//----------------------------------------------------------------------------------------------------
void Map::Render() const
{
    if (g_theGame->IsAttractMode()) return;

    RenderTiles();
    RenderTileHeatMap();
    DebugRenderTileIndex();

    RenderEntities();
}

//----------------------------------------------------------------------------------------------------
void Map::DebugRender() const
{
    if (g_theGame->IsAttractMode()) return;

    if (!g_theGame->IsDebugRendering()) return;

    DebugRenderEntities();

    if (m_currentSelectedEntity) DebugDrawRing(m_currentSelectedEntity->m_position, 1.f, 0.05f, Rgba8::BLUE);
}

//----------------------------------------------------------------------------------------------------
IntVec2 const Map::GetTileCoordsFromWorldPos(Vec2 const& worldPos) const
{
    int const tileX = RoundDownToInt(worldPos.x);
    int const tileY = RoundDownToInt(worldPos.y);

    return IntVec2(tileX, tileY);
}

//----------------------------------------------------------------------------------------------------
Vec2 const Map::GetWorldPosFromTileCoords(IntVec2 const& tileCoords) const
{
    constexpr float halfTileWidth = 0.5f;
    float const     worldX        = static_cast<float>(tileCoords.x) + halfTileWidth;
    float const     worldY        = static_cast<float>(tileCoords.y) + halfTileWidth;

    return Vec2(worldX, worldY);
}

//----------------------------------------------------------------------------------------------------
Entity* Map::SpawnNewEntity(EntityType const    type,
                            EntityFaction const faction,
                            Vec2 const&         position,
                            float const         orientationDegrees)
{
    Entity* newEntity = CreateNewEntity(type, faction);

    AddEntityToMap(newEntity, position, orientationDegrees);

    return newEntity;
}

//----------------------------------------------------------------------------------------------------
bool Map::HasLineOfSight(Vec2 const& startPos,
                         Vec2 const& endPos,
                         float const sightRange) const
{
    float const distSquared      = GetDistanceSquared2D(startPos, endPos);
    float const sighRangeSquared = sightRange * sightRange;

    if (distSquared >= sighRangeSquared) return false;

    Vec2 const  fwdNormal = (endPos - startPos).GetNormalized();
    float const maxDist   = GetDistance2D(startPos, endPos);
    Ray2 const  ray       = Ray2(startPos, fwdNormal, maxDist);

    return !RaycastVsTiles(ray).m_didImpact;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsTileSolid(IntVec2 const& tileCoords) const
{
    if (IsTileCoordsOutOfBounds(tileCoords)) return true;

    int const tileIndex = tileCoords.y * m_dimensions.x + tileCoords.x;

    if (tileIndex >= 0 &&
        tileIndex < static_cast<int>(m_tiles.size()))
    {
        Tile const& tile = m_tiles[tileIndex];

        return TileDefinition::GetTileDefByName(tile.m_name)->IsSolid() == true;
    }

    return false;
}

bool Map::IsTileWater(IntVec2 const& tileCoords) const
{
    if (IsTileCoordsOutOfBounds(tileCoords)) return true;

    int const tileIndex = tileCoords.y * m_dimensions.x + tileCoords.x;

    if (tileIndex >= 0 &&
        tileIndex < static_cast<int>(m_tiles.size()))
    {
        Tile const& tile = m_tiles[tileIndex];

        return TileDefinition::GetTileDefByName(tile.m_name)->IsWater() == true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsPointInSolid(Vec2 const& point) const
{
    IntVec2 const tileCoords = GetTileCoordsFromWorldPos(point);

    return IsTileSolid(tileCoords);
}

//----------------------------------------------------------------------------------------------------
void Map::UpdateEntities(float const deltaSeconds) const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        Entity* entity = m_allEntities[entityIndex];

        if (!entity) continue;

        entity->Update(deltaSeconds);
    }
}

//----------------------------------------------------------------------------------------------------
void Map::RenderTiles() const
{
    VertexList_PCU tileVertices;

    tileVertices.reserve(static_cast<size_t>(3) * 2 * m_dimensions.x * m_dimensions.y);

    for (String const& tileName : TileDefinition::GetTileNames())
    {
        TileDefinition const*  tileDef   = TileDefinition::GetTileDefByName(tileName);
        SpriteDefinition const spriteDef = tileDef->GetSpriteDef();

        Vec2 const uvAtMins = spriteDef.GetUVsMins();
        Vec2 const uvAtMaxs = spriteDef.GetUVsMaxs();

        for (Tile const& tile : m_tiles)
        {
            if (tile.m_name == tileName)
            {
                Vec2 const mins(static_cast<float>(tile.m_coords.x), static_cast<float>(tile.m_coords.y));
                Vec2 const maxs = mins + Vec2::ONE;

                AddVertsForAABB2D(tileVertices, AABB2(mins, maxs), tileDef->GetTintColor(), uvAtMins, uvAtMaxs);
            }
        }
    }

    g_theRenderer->BindTexture(&g_theGame->GetTileSpriteSheet()->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(tileVertices.size()), tileVertices.data());
}

//----------------------------------------------------------------------------------------------------
void Map::RenderEntities() const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        if (Entity const* entity = m_allEntities[entityIndex])
        {
            entity->Render();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::RenderTileHeatMap() const
{
    if (m_currentTileHeatMapIndex == -1) return;

    AABB2 const totalBounds = GetMapBound();

    VertexList_PCU verts;

    if (m_currentTileHeatMapIndex == 3)
    {
        if (!m_currentSelectedEntity->m_heatMap) return;

        m_currentSelectedEntity->m_heatMap->AddVertsForDebugDraw(verts, totalBounds);
    }
    else
    {
        if (!m_tileHeatMaps[m_currentTileHeatMapIndex]) return;

        m_tileHeatMaps[m_currentTileHeatMapIndex]->AddVertsForDebugDraw(verts, totalBounds);
    }

    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void Map::RenderTileHeatMapText() const
{
    if (g_theGame->IsAttractMode()) return;

    if (m_currentTileHeatMapIndex == -1) return;

    VertexList_PCU textVerts;
    AABB2      box = AABB2(Vec2(0.f, 780.f), Vec2(1600.f, 800.f));

    VertexList_PCU boxVerts;

    AddVertsForAABB2D(boxVerts, box, Rgba8::BLACK);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(boxVerts.size()), boxVerts.data());

    switch (m_currentTileHeatMapIndex)
    {
    case 0:
        g_theBitmapFont->AddVertsForTextInBox2D(textVerts, "Debug Heat Map: Distance Map from start (F6 for next mode)", box, 1.f, Rgba8::WHITE, 1.f, Vec2(0, 1));
        break;

    case 1:
        g_theBitmapFont->AddVertsForTextInBox2D(textVerts, "Debug Heat Map: Solid Map for amphibians (F6 for next mode)", box, 0.5f);
        break;

    case 2:
        g_theBitmapFont->AddVertsForTextInBox2D(textVerts, "Debug Heat Map: Solid Map for land-based (F6 for next mode)", box, 0.5f);
        break;

    case 3:
        g_theBitmapFont->AddVertsForTextInBox2D(textVerts, "Debug Heat Map: Distance Map to selected Entity's goal (F6 for next mode)", box, 0.5f);
        break;
    }

    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}

//----------------------------------------------------------------------------------------------------
void Map::DebugRenderEntities() const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        if (Entity const* entity = m_allEntities[entityIndex]) entity->DebugRender();
    }
}

//----------------------------------------------------------------------------------------------------
void Map::DebugRenderTileIndex() const
{
    if (m_currentTileHeatMapIndex == -1) return;

    for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
    {
        for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
        {
            TileHeatMap const* heatMap;

            if (m_currentTileHeatMapIndex == 3)
            {
                if (!m_currentSelectedEntity->m_heatMap) return;

                heatMap = m_currentSelectedEntity->m_heatMap;
            }
            else
            {
                if (!m_tileHeatMaps[m_currentTileHeatMapIndex]) return;

                heatMap = m_tileHeatMaps[m_currentTileHeatMapIndex];
            }

            float const value = heatMap->GetValueAtCoords(tileX, tileY);
            VertexList_PCU  textVerts;

            g_theBitmapFont->AddVertsForText2D(textVerts, std::to_string(static_cast<int>(value)), Vec2(tileX, tileY), 0.2f, Rgba8::WHITE);
            g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
            g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::InitializeTileHeatMaps()
{
    m_tileHeatMaps.reserve(4);

    for (int i = 0; i < 4; ++i)
    {
        m_tileHeatMaps.push_back(new TileHeatMap(m_dimensions, 999.f));
    }
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateAllTiles()
{
    printf("( Map%d ) Start  | GenerateAllTiles\n", m_mapDef->GetIndex());

    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            m_tiles.emplace_back();
        }
    }

    MapDefinition const* mapDef = MapDefinition::s_mapDefinitions[GetMapIndex()];

    GenerateTilesByType("Stone");
    GenerateWormTiles(mapDef->GetWorm01TileName(), mapDef->GetWorm01Num(), mapDef->GetWorm01Length());
    GenerateWormTiles(mapDef->GetWorm02TileName(), mapDef->GetWorm02Num(), mapDef->GetWorm02Length());
    GenerateWormTiles(mapDef->GetWorm03TileName(), mapDef->GetWorm03Num(), mapDef->GetWorm03Length());

    GenerateTilesByType("Floor");

    GenerateLShapeTiles(2, 2, 5, 5, false);
    GenerateLShapeTiles(m_dimensions.x - 9, m_dimensions.y - 9, 7, 7, true);
    GenerateStartPosTile();
    GenerateExitPosTile();

    if (!IsValidMap(IntVec2::ONE, m_exitPosition, 100))
    {
        ERROR_AND_DIE("Failed to GenerateAllTiles!")
    }

    TileHeatMap const heatMap(m_dimensions, 999.f);
    PopulateDistanceField(heatMap, IntVec2::ONE, 999.f);
    ConvertUnreachableTilesToSolid(heatMap, "Stone");

    printf("( Map%d ) Finish | GenerateAllTiles\n", m_mapDef->GetIndex());
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateTilesByType(String const& tileName)
{
    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            if (tileName == "Floor")
            {
                if (!IsEdgeTile(x, y) &&
                    IsTileCoordsInLShape(x, y))
                {
                    SetTileAtCoords(tileName, x, y);
                }
            }

            if (tileName == "Stone")
            {
                if (IsEdgeTile(x, y) ||
                    !IsTileCoordsInLShape(x, y))
                {
                    SetTileAtCoords(tileName, x, y);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateWormTiles(String const& wormTileName, int const numWorms, int const wormLength)
{
    for (int i = 0; i < numWorms; ++i)
    {
        IntVec2 wormPosition = RollRandomTileCoords();

        if (!IsEdgeTile(wormPosition.x, wormPosition.y))
        {
            SetTileAtCoords(wormTileName, wormPosition.x, wormPosition.y);
        }

        for (int j = 0; j < wormLength; ++j)
        {
            IntVec2 const direction   = RollRandomCardinalDirection();
            IntVec2 const newPosition = wormPosition + direction;

            if (!IsTileCoordsOutOfBounds(newPosition) &&
                !IsEdgeTile(newPosition.x, newPosition.y))
            {
                wormPosition = newPosition;
                SetTileAtCoords(wormTileName, wormPosition.x, wormPosition.y);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateLShapeTiles(int const  tileCoordX,
                              int const  tileCoordY,
                              int const  width,
                              int const  height,
                              bool const isBottomLeft)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (isBottomLeft)
            {
                if (y == 0 || x == 0)
                {
                    SetTileAtCoords("Stone", tileCoordX + x, tileCoordY + y);
                }
            }
            else
            {
                if (y == height - 1 || x == width - 1)
                {
                    SetTileAtCoords("Stone", tileCoordX + x, tileCoordY + y);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateStartPosTile()
{
    SetTileAtCoords("Start", m_startPosition.x, m_startPosition.y);
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateExitPosTile()
{
    SetTileAtCoords("Exit", m_exitPosition.x, m_exitPosition.y);
}

//----------------------------------------------------------------------------------------------------
void Map::SetTileAtCoords(String const& tileName, int const tileX, int const tileY)
{
    int const tileIndex = tileY * m_dimensions.x + tileX;

    m_tiles[tileIndex].m_coords = IntVec2(tileX, tileY);
    m_tiles[tileIndex].m_name   = tileName;
}

//----------------------------------------------------------------------------------------------------
void Map::ConvertUnreachableTilesToSolid(TileHeatMap const& heatMap, String const& tileName)
{
    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            IntVec2 tileCoords(x, y);

            if (!IsTileSolid(tileCoords) &&
                heatMap.GetValueAtCoords(IntVec2(x, y)) == 999.f)
            {
                SetTileAtCoords(tileName, x, y);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool Map::IsEdgeTile(int const x, int const y) const
{
    return
        x == 0 ||
        x == m_dimensions.x - 1 ||
        y == 0 ||
        y == m_dimensions.y - 1;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsTileCoordsInLShape(int const x, int const y) const
{
    bool const inLeftLShape  = x <= 5 && y <= 5;
    bool const inRightLShape = x >= m_dimensions.x - 8 && y >= m_dimensions.y - 8;

    return inLeftLShape || inRightLShape;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsTileCoordsOutOfBounds(IntVec2 const& tileCoords) const
{
    return
        tileCoords.x < 0 ||
        tileCoords.x >= m_dimensions.x ||
        tileCoords.y < 0 ||
        tileCoords.y >= m_dimensions.y;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsWorldPosOccupied(Vec2 const& position) const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        if (m_allEntities[entityIndex]->m_position == position)
        {
            return true;
        }
    }
    return false;
}

bool Map::IsWorldPosOccupiedByEntity(Vec2 const& position, EntityType const type) const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        // Check if the position matches
        if (m_allEntities[entityIndex]->m_position == position)
        {
            // Optionally filter by entity type
            if (m_allEntities[entityIndex]->m_type == type)
            {
                return true;
            }
        }
    }

    return false;
}

bool Map::IsValidMap(IntVec2 const& startCoords, IntVec2 const& exitCoords, int const maxAttempts)
{
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        TileHeatMap heatMap(m_dimensions, 999.f);
        PopulateDistanceField(heatMap, startCoords, 999.f);

        if (heatMap.GetValueAtCoords(exitCoords) != 999.f)
        {
            return true;
        }

        GenerateAllTiles();
    }

    ERROR_AND_DIE("Failed to generate a valid map after maximum attempts!")
}


//----------------------------------------------------------------------------------------------------
AABB2 const Map::GetTileBounds(IntVec2 const& tileCoords) const
{
    if (IsTileCoordsOutOfBounds(tileCoords))
        ERROR_AND_DIE("tileCoords is out of bound")

    float const minsX = static_cast<float>(tileCoords.x);
    float const minsY = static_cast<float>(tileCoords.y);
    Vec2 const  mins(minsX, minsY);
    Vec2 const  maxs(mins + Vec2::ONE);

    return AABB2(mins, maxs);
}

//----------------------------------------------------------------------------------------------------
AABB2 const Map::GetTileBounds(int const tileIndex) const
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(m_tiles.size()))
        ERROR_AND_DIE("tileIndex is out of bound")

    int const     tileX = tileIndex % m_dimensions.x;
    int const     tileY = tileIndex / m_dimensions.x;
    IntVec2 const tileCoords(tileX, tileY);

    return GetTileBounds(tileCoords);
}

//----------------------------------------------------------------------------------------------------
IntVec2 Map::RollRandomTileCoords() const
{
    int const randomX = g_theRNG->RollRandomIntInRange(0, m_dimensions.x - 1);
    int const randomY = g_theRNG->RollRandomIntInRange(0, m_dimensions.y - 1);

    return IntVec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
IntVec2 Map::RollRandomTraversableTileCoords(TileHeatMap const& heatMap, IntVec2 const& startCoords) const
{
    // 先填充距離場
    PopulateDistanceFieldForEntity(heatMap, startCoords, 999.f);

    // 儲存可到達的座標
    std::vector<IntVec2> traversableCoords;

    // 遍歷整個地圖的每個座標
    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            IntVec2 currentCoords(x, y);

            // 檢查該座標是否可到達
            if (IsTileSolid(currentCoords) ||
                IsWorldPosOccupiedByEntity(Vec2(x, y) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO) ||
                heatMap.GetValueAtCoords(currentCoords) == 999.f)
                continue;

            traversableCoords.push_back(currentCoords);
        }
    }

    // 如果沒有可到達的座標，回傳一個無效座標或進行處理
    if (traversableCoords.empty())
    {
        ERROR_AND_DIE("No traversable tiles found!");
    }

    // 從可到達的座標中隨機選擇一個
    int randomIndex = g_theRNG->RollRandomIntInRange(0, static_cast<int>(traversableCoords.size() - 1));
    return traversableCoords[randomIndex];
}

//----------------------------------------------------------------------------------------------------
IntVec2 Map::RollRandomCardinalDirection() const
{
    switch (g_theRNG->RollRandomIntInRange(0, 3))
    {
    case 0:
        return IntVec2(0, 1);
    case 1:
        return IntVec2(1, 0);
    case 2:
        return IntVec2(0, -1);
    case 3:
        return IntVec2(-1, 0);
    }
    return IntVec2::ZERO;
}

//----------------------------------------------------------------------------------------------------
void Map::GenerateHeatMaps(TileHeatMap const& heatMap) const
{
    printf("( Map%d ) Start  | GenerateHeatMaps\n", m_mapDef->GetIndex());

    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            IntVec2 tileCoords(x, y);

            if (IsTileSolid(tileCoords))
            {
                heatMap.SetValueAtCoords(tileCoords, 999.f);
            }
        }
    }

    printf("( Map%d ) Finish | GenerateHeatMaps\n", m_mapDef->GetIndex());
}

//----------------------------------------------------------------------------------------------------
void Map::PopulateDistanceField(TileHeatMap const& heatMap, IntVec2 const& startCoords, float const specialValue) const
{
    // printf("( Map%d ) Start  | GenerateDistanceField\n", m_mapDef->GetIndex());

    heatMap.SetValueAtAllTiles(specialValue);
    heatMap.SetValueAtCoords(startCoords, 0.f);

    float currentSearchValue = 0.f;
    bool  isStillGoing       = true;

    while (isStillGoing)    // For each pass, assume we're done UNLESS something changes
    {
        isStillGoing = false;

        for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
        {
            for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
            {
                IntVec2     tileCoords(tileX, tileY);
                float const value = heatMap.GetValueAtCoords(tileX, tileY);

                if (std::fabs(value - currentSearchValue) < FLOAT_MIN)
                {
                    // Found a search value ! Spread to cardinal neighbors...
                    IntVec2     n               = tileCoords + IntVec2(0, 1);
                    IntVec2     e               = tileCoords + IntVec2(1, 0);
                    IntVec2     w               = tileCoords + IntVec2(-1, 0);
                    IntVec2     s               = tileCoords + IntVec2(0, -1);
                    float const nextSearchValue = currentSearchValue + 1.f;

                    if (!IsTileCoordsOutOfBounds(e) && !IsTileSolid(e) && !IsTileWater(e) && heatMap.GetValueAtCoords(e) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(e, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(n) && !IsTileSolid(n) && !IsTileWater(n) && heatMap.GetValueAtCoords(n) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(n, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(s) && !IsTileSolid(s) && !IsTileWater(s) && heatMap.GetValueAtCoords(s) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(s, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(w) && !IsTileSolid(w) && !IsTileWater(w) && heatMap.GetValueAtCoords(w) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(w, nextSearchValue);
                        isStillGoing = true;
                    }
                }
            }
        }
        currentSearchValue++;
    }

    // printf("( Map%d ) Finish | GenerateDistanceField\n", m_mapDef->GetIndex());
}

void Map::PopulateDistanceFieldForEntity(TileHeatMap const& heatMap, IntVec2 const& startCoords, float specialValue) const
{
    // printf("( Map%d ) Start  | GenerateDistanceField\n", m_mapDef->GetIndex());

    heatMap.SetValueAtAllTiles(specialValue);
    heatMap.SetValueAtCoords(startCoords, 0.f);

    float currentSearchValue = 0.f;
    bool  isStillGoing       = true;

    while (isStillGoing)    // For each pass, assume we're done UNLESS something changes
    {
        isStillGoing = false;

        for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
        {
            for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
            {
                IntVec2     tileCoords(tileX, tileY);
                float const value = heatMap.GetValueAtCoords(tileX, tileY);

                if (std::fabs(value - currentSearchValue) < FLOAT_MIN)
                {
                    // Found a search value ! Spread to cardinal neighbors...
                    IntVec2     n               = tileCoords + IntVec2(0, 1);
                    IntVec2     e               = tileCoords + IntVec2(1, 0);
                    IntVec2     w               = tileCoords + IntVec2(-1, 0);
                    IntVec2     s               = tileCoords + IntVec2(0, -1);
                    float const nextSearchValue = currentSearchValue + 1.f;

                    if (!IsTileCoordsOutOfBounds(e) && !IsTileSolid(e) && !IsTileWater(e) && !IsWorldPosOccupiedByEntity(Vec2(e) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO) && heatMap.GetValueAtCoords(e) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(e, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(n) && !IsTileSolid(n) && !IsTileWater(n) && !IsWorldPosOccupiedByEntity(Vec2(n) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO) && heatMap.GetValueAtCoords(n) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(n, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(s) && !IsTileSolid(s) && !IsTileWater(s) && !IsWorldPosOccupiedByEntity(Vec2(s) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO) && heatMap.GetValueAtCoords(s) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(s, nextSearchValue);
                        isStillGoing = true;
                    }

                    if (!IsTileCoordsOutOfBounds(w) && !IsTileSolid(w) && !IsTileWater(w) && !IsWorldPosOccupiedByEntity(Vec2(w) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO) && heatMap.GetValueAtCoords(w) > nextSearchValue)
                    {
                        heatMap.SetValueAtCoords(w, nextSearchValue);
                        isStillGoing = true;
                    }
                }
            }
        }
        currentSearchValue++;
    }

    // printf("( Map%d ) Finish | GenerateDistanceField\n", m_mapDef->GetIndex());
}


//----------------------------------------------------------------------------------------------------
void Map::PopulateDistanceFieldForLandBased(TileHeatMap const& heatMap) const
{
    for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
    {
        for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
        {
            Vec2 worldPos = Vec2(tileX, tileY) + Vec2(0.5f, 0.5f);

            if (IsWorldPosOccupiedByEntity(worldPos, ENTITY_TYPE_SCORPIO))
            {
                continue;
            }

            IntVec2 tileCoords(tileX, tileY);

            if (!IsTileCoordsOutOfBounds(tileCoords) && !IsTileSolid(tileCoords))
            {
                heatMap.SetValueAtCoords(tileCoords, 0.f);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::PopulateDistanceFieldForAmphibian(TileHeatMap const& heatMap) const
{
    for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
    {
        for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
        {
            Vec2 worldPos = Vec2(tileX, tileY) + Vec2(0.5f, 0.5f);

            if (IsWorldPosOccupiedByEntity(worldPos, ENTITY_TYPE_SCORPIO))
            {
                continue;
            }

            IntVec2 tileCoords(tileX, tileY);

            if (!IsTileCoordsOutOfBounds(tileCoords) &&
                !IsTileSolid(tileCoords) ||
                IsTileWater(tileCoords))
            {
                heatMap.SetValueAtCoords(tileCoords, 0.f);
            }
        }
    }
}

void Map::PopulateDistanceFieldToPosition(TileHeatMap const& heatMap, IntVec2 const& playerCoords) const
{
    // printf("( Map%d ) Start  | GenerateDistanceFieldToPlayerPosition\n", m_mapDef->GetIndex());

    heatMap.SetValueAtAllTiles(999.f);
    heatMap.SetValueAtCoords(playerCoords, 0.f);

    std::queue<IntVec2> openList;
    openList.push(playerCoords);

    while (!openList.empty())
    {
        IntVec2 currentTile = openList.front();
        openList.pop();

        float const currentDistance = heatMap.GetValueAtCoords(currentTile);

        // ???? tile ??????? (N, E, S, W)
        IntVec2 neighbors[] = {
            currentTile + IntVec2(0, 1),  // ? (N)
            currentTile + IntVec2(1, 0),  // ? (E)
            currentTile + IntVec2(0, -1), // ? (S)
            currentTile + IntVec2(-1, 0)  // ? (W)
        };

        for (IntVec2 const& neighbor : neighbors)
        {
            if (IsTileCoordsOutOfBounds(neighbor) || IsTileSolid(neighbor) || IsWorldPosOccupiedByEntity(Vec2(neighbor) + Vec2(0.5f, 0.5f), ENTITY_TYPE_SCORPIO))
            {
                continue; // ??????????????
            }

            float const neighborDistance = heatMap.GetValueAtCoords(neighbor);
            float const newDistance      = currentDistance + 1.f; // ??? tile ??? +1

            if (newDistance < neighborDistance) // ?????????
            {
                heatMap.SetValueAtCoords(neighbor, newDistance);
                openList.push(neighbor); // ??????????????
            }
        }
    }

    // printf("( Map%d ) Finish | GenerateDistanceFieldToPlayerPosition\n", m_mapDef->GetIndex());
}

std::vector<Vec2> Map::GenerateEntityPathToGoal(TileHeatMap const& heatMap, Vec2 const& start, Vec2 const& goal) const
{
    // 初始化熱圖，設置高初始值
    // TileHeatMap heatMap(GetMapDimension(), 999.f);

    // 計算目標在地圖上的座標
    IntVec2 goalCoords = GetTileCoordsFromWorldPos(goal);

    // 填充距離場，用於計算路徑
    PopulateDistanceFieldToPosition(heatMap, goalCoords);

    // 設置當前位置
    IntVec2           currentCoords = GetTileCoordsFromWorldPos(start);
    std::vector<Vec2> path;
    path.reserve((int)(m_dimensions.x * m_dimensions.y));

    while (currentCoords != goalCoords)
    {
        path.push_back(GetWorldPosFromTileCoords(currentCoords));

        // 找到熱值最低的相鄰 tile
        IntVec2 bestNeighbor = currentCoords;
        float   lowestHeat   = heatMap.GetValueAtCoords(currentCoords);

        for (IntVec2 const& offset : {IntVec2(-1, 0), IntVec2(1, 0), IntVec2(0, -1), IntVec2(0, 1)})
        {
            IntVec2 neighbor = currentCoords + offset;
            float   heat     = heatMap.GetValueAtCoords(neighbor);

            if (heat < lowestHeat)
            {
                lowestHeat   = heat;
                bestNeighbor = neighbor;
            }
        }

        // 更新當前位置
        currentCoords = bestNeighbor;
    }

    // 添加最終目標點
    path.push_back(goal);
    std::reverse(path.begin(), path.end());
    return path;
}

bool Map::RaycastHitsImpassable(Vec2 const& currentPos, Vec2 const& nextNextPos)
{
    Vec2            direction       = nextNextPos - currentPos;
    Ray2            ray             = Ray2(currentPos, direction.GetNormalized(), GetDistance2D(currentPos, nextNextPos));
    RaycastResult2D raycastResult2D = RaycastVsTiles(ray);

    if (raycastResult2D.m_didImpact) return true;

    return false;
}

//----------------------------------------------------------------------------------------------------
Entity* Map::CreateNewEntity(EntityType const type, EntityFaction const faction)
{
    switch (type)
    {
    case ENTITY_TYPE_PLAYER_TANK:
        return new PlayerTank(this, type, faction);
    case ENTITY_TYPE_SCORPIO:
        return new Scorpio(this, type, faction);
    case ENTITY_TYPE_LEO:
        return new Leo(this, type, faction);
    case ENTITY_TYPE_ARIES:
        return new Aries(this, type, faction);
    case ENTITY_TYPE_BULLET:
        return new Bullet(this, type, faction);
    case ENTITY_TYPE_EXPLOSION:
        return new Explosion(this, type, faction);
    case ENTITY_TYPE_DEBRIS:
        return new Debris(this, type, faction);
    case ENTITY_TYPE_UNKNOWN:
        ERROR_AND_DIE(Stringf("Unknown entity type #%i\n", type))
    case NUM_ENTITY_TYPES:
        ERROR_AND_DIE(Stringf("Unknown entity type #%i\n", type))
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void Map::AddEntityToMap(Entity* entity, Vec2 const& position, float const orientationDegrees)
{
    entity->m_map                = this;
    entity->m_position           = position;
    entity->m_orientationDegrees = orientationDegrees;

    AddEntityToList(entity, m_allEntities);
    AddEntityToList(entity, m_entitiesByType[entity->m_type]);

    if (IsBullet(entity)) AddEntityToList(entity, m_bulletsByFaction[entity->m_faction]);

    if (IsAgent(entity)) AddEntityToList(entity, m_agentsByFaction[entity->m_faction]);
}

//----------------------------------------------------------------------------------------------------
void Map::AddEntityToList(Entity* entity, EntityList& entityList)
{
    entityList.push_back(entity);
}

//----------------------------------------------------------------------------------------------------
void Map::RemoveEntityFromMap(Entity* entity)
{
    RemoveEntityFromList(entity, m_allEntities);
    RemoveEntityFromList(entity, m_entitiesByType[entity->m_type]);

    if (IsAgent(entity)) RemoveEntityFromList(entity, m_agentsByFaction[entity->m_faction]);

    if (IsBullet(entity)) RemoveEntityFromList(entity, m_bulletsByFaction[entity->m_faction]);

    entity->m_map = nullptr;
}

//----------------------------------------------------------------------------------------------------
void Map::RemoveEntityFromList(Entity const* entity, EntityList& entityList)
{
    for (EntityList::iterator it = entityList.begin(); it != entityList.end(); ++it)
    {
        if (*it == entity)
        {
            entityList.erase(it);
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::DeleteGarbageEntities()
{
    for (Entity* entity : m_allEntities)
    {
        if (entity->m_isGarbage)
        {
            RemoveEntityFromMap(entity);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::SpawnNewNPCs()
{
    printf("( Map%d ) Start  | SpawnNewNPCs\n", m_mapDef->GetIndex());

    for (int i = 0; i < m_dimensions.x * m_dimensions.y; ++i)
    {
        IntVec2 const randomTileCoords = RollRandomTileCoords();

        if (IsEdgeTile(randomTileCoords.x, randomTileCoords.y) ||
            IsTileSolid(IntVec2(randomTileCoords.x, randomTileCoords.y)) ||
            IsTileCoordsInLShape(randomTileCoords.x, randomTileCoords.y))
            continue;

        Vec2 const worldPosition(static_cast<float>(randomTileCoords.x) + 0.5f, static_cast<float>(randomTileCoords.y) + 0.5f);

        if (IsWorldPosOccupied(worldPosition)) continue;

        switch (g_theRNG->RollRandomIntInRange(0, 3))
        {
        case 0:
            if (g_theRNG->RollRandomFloatZeroToOne() < m_mapDef->GetScorpioSpawnPercentage()) SpawnNewEntity(ENTITY_TYPE_SCORPIO, ENTITY_FACTION_EVIL, worldPosition, 0.f);

            break;

        case 1:
            if (g_theRNG->RollRandomFloatZeroToOne() < m_mapDef->GetLeoSpawnPercentage()) SpawnNewEntity(ENTITY_TYPE_LEO, ENTITY_FACTION_EVIL, worldPosition, 0.f);

            break;

        case 2:
            if (g_theRNG->RollRandomFloatZeroToOne() < m_mapDef->GetAriesSpawnPercentage()) SpawnNewEntity(ENTITY_TYPE_ARIES, ENTITY_FACTION_EVIL, worldPosition, 0.f);

            break;
        }
    }

    printf("( Map%d ) Finish | SpawnNewNPCs\n", m_mapDef->GetIndex());
}

//----------------------------------------------------------------------------------------------------
bool Map::IsBullet(Entity const* entity) const
{
    return
        entity->m_type == ENTITY_TYPE_BULLET;
}

//----------------------------------------------------------------------------------------------------
bool Map::IsAgent(Entity const* entity) const
{
    return
        entity->m_type != ENTITY_TYPE_BULLET &&
        entity->m_type != ENTITY_TYPE_PLAYER_TANK;
}

//----------------------------------------------------------------------------------------------------
void Map::PushEntitiesOutOfWalls() const
{
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
    {
        if (IsBullet(m_allEntities[entityIndex])) continue;

        if (g_theGame->IsNoClip() && m_allEntities[entityIndex]->m_type == ENTITY_TYPE_PLAYER_TANK) continue;

        PushEntityOutOfSolidTiles(m_allEntities[entityIndex]);
    }
}

//----------------------------------------------------------------------------------------------------
void Map::PushEntityOutOfSolidTiles(Entity* entity) const
{
    IntVec2 const myTileCoords = GetTileCoordsFromWorldPos(entity->m_position);

    // Push out of cardinal neighbors (NSEW) first
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(1, 0));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(0, 1));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(-1, 0));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(0, -1));

    // Push out of diagonal neighbors second
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(1, 1));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(-1, 1));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(-1, -1));
    PushEntityOutOfTileIfSolid(entity, myTileCoords + IntVec2(1, -1));
}

//----------------------------------------------------------------------------------------------------
void Map::PushEntityOutOfTileIfSolid(Entity* entity, IntVec2 const& tileCoords) const
{
    if (!IsTileSolid(tileCoords)) return;

    if (IsTileCoordsOutOfBounds(tileCoords)) return;

    AABB2 const aabb2Box = GetTileBounds(tileCoords);

    PushDiscOutOfAABB2D(entity->m_position, entity->m_physicsRadius, aabb2Box);
}

//----------------------------------------------------------------------------------------------------
void Map::PushEntitiesOutOfEachOther(EntityList const& entityListA, EntityList const& entityListB) const
{
    for (Entity* entityA : entityListA)
    {
        if (!entityA) continue;

        for (Entity* entityB : entityListB)
        {
            if (!entityB) continue;

            if (entityA == entityB) continue;

            bool const canAPushB = entityA->m_doesPushEntities && entityB->m_isPushedByEntities;
            bool const canBPushA = entityB->m_doesPushEntities && entityA->m_isPushedByEntities;

            if (canAPushB &&
                canBPushA)
            {
                PushDiscsOutOfEachOther2D(entityA->m_position,
                                          entityA->m_physicsRadius,
                                          entityB->m_position,
                                          entityB->m_physicsRadius);
            }

            if (!canAPushB &&
                canBPushA)
            {
                PushDiscOutOfDisc2D(entityA->m_position,
                                    entityA->m_physicsRadius,
                                    entityB->m_position,
                                    entityB->m_physicsRadius);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Map::CheckEntityVsEntityCollision(EntityList const& entityListA, EntityList const& entityListB)
{
    for (Entity* entityA : entityListA)
    {
        if (!entityA) continue;

        if (entityA->m_isDead) continue;

        for (Entity* entityB : entityListB)
        {
            if (!entityB) continue;

            if (entityB->m_isDead) continue;

            if (IsBullet(entityB)) continue;

            if (entityA == entityB) continue;

            if (DoDiscsOverlap2D(entityA->m_position, entityA->m_physicsRadius, entityB->m_position, entityB->m_physicsRadius))
            {
                if (entityA->m_faction == entityB->m_faction) continue;

                if (entityB->m_type == ENTITY_TYPE_ARIES)
                {
                    if (IsPointInsideDirectedSector2D(entityA->m_position, entityB->m_position, entityB->m_velocity.GetNormalized(), 90.f, entityB->m_physicsRadius * 1.5f))
                    {
                        RaycastResult2D const raycastResult2D   = RaycastVsDisc2D(entityA->m_position, entityA->m_velocity.GetNormalized(), entityA->m_velocity.GetLength(), entityB->m_position, entityB->m_physicsRadius);
                        Vec2 const            reflectedVelocity = entityA->m_velocity.GetReflected(raycastResult2D.m_impactNormal);

                        entityA->m_orientationDegrees = Atan2Degrees(reflectedVelocity.y, reflectedVelocity.x);
                        entityA->m_health--;
                        g_theAudio->StartSound(g_theGame->GetEnemyHitSoundID());
                        return;
                    }
                }

                entityA->m_health--;
                entityB->m_health--;

                if (entityB->m_type == ENTITY_TYPE_PLAYER_TANK)
                {
                    g_theAudio->StartSound(g_theGame->GetPlayerTankHitSoundID());
                }
                else
                {
                    g_theAudio->StartSound(g_theGame->GetEnemyHitSoundID());
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D Map::RaycastVsTiles(Ray2 const& ray) const
{
    RaycastResult2D raycastResult;
    raycastResult.m_rayForwardNormal = ray.m_forwardNormal;
    raycastResult.m_rayStartPosition = ray.m_startPosition;
    raycastResult.m_rayMaxLength     = ray.m_maxLength;
    raycastResult.m_didImpact        = false;

    constexpr float stepSize   = 0.01f;
    Vec2            currentPos = ray.m_startPosition;


    // Calculate the number of steps needed
    const int numSteps = static_cast<int>(ray.m_maxLength / stepSize);

    for (int i = 0; i < numSteps; ++i)
    {
        const float t            = static_cast<float>(i) * stepSize;
        currentPos               = ray.m_startPosition + ray.m_forwardNormal * t;
        Vec2          prePos     = currentPos - ray.m_forwardNormal * stepSize;
        IntVec2 const tileCoords = GetTileCoordsFromWorldPos(currentPos);

        // Check bounds
        if (IsTileCoordsOutOfBounds(tileCoords))
        {
            raycastResult.m_didImpact  = true;
            raycastResult.m_impactLength = t;
            raycastResult.m_impactPosition  = currentPos;

            return raycastResult; // Out of bounds is considered blocking
        }

        // Check tile blocking

        if (IsTileSolid(tileCoords) && !IsTileWater(tileCoords))
        {
            raycastResult.m_didImpact     = true;
            raycastResult.m_impactLength    = t;
            raycastResult.m_impactPosition     = currentPos;
            IntVec2 const impactFwdNormal = GetTileCoordsFromWorldPos(currentPos) - GetTileCoordsFromWorldPos(prePos);
            raycastResult.m_impactNormal  = Vec2(impactFwdNormal);

            return raycastResult;
        }
    }

    return raycastResult;
}
