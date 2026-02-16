//----------------------------------------------------------------------------------------------------
// Map.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Entity.hpp"
#include "Game/MapDefinition.hpp"

//----------------------------------------------------------------------------------------------------
class TileHeatMap;
struct Tile;

//-----------------------------------------------------------------------------------------------
class Map
{
public:
    explicit Map(MapDefinition const& mapDef);
    ~Map();

    void Update(float deltaSeconds);
    void Render() const;
    void DebugRender() const;
    void RenderTileHeatMapText() const;

    // Accessors (const methods)
    Vec2 const    GetWorldPosFromTileCoords(IntVec2 const& tileCoords) const;
    IntVec2 const GetTileCoordsFromWorldPos(Vec2 const& worldPos) const;
    IntVec2 const GetMapDimension() const { return m_dimensions; }
    IntVec2 const GetMapExitPosition() const { return m_exitPosition; }
    AABB2 const   GetMapBound() const { return AABB2(IntVec2::ZERO, m_dimensions); }
    int           GetMapIndex() const { return m_mapDef->GetIndex(); }
    int           GetTileNums() const { return m_dimensions.x * m_dimensions.y; }

    // Mutators (non-const methods)
    Entity* SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 const& position, float orientationDegrees);
    void    AddEntityToMap(Entity* entity, Vec2 const& position, float orientationDegrees);
    void    RemoveEntityFromMap(Entity* entity);

    // Helpers
    RaycastResult2D RaycastVsTiles(Ray2 const& ray) const;
    bool            HasLineOfSight(Vec2 const& startPos, Vec2 const& endPos, float sightRange) const;
    bool            IsTileSolid(IntVec2 const& tileCoords) const;
    bool            IsTileWater(IntVec2 const& tileCoords) const;
    bool            IsPointInSolid(Vec2 const& point) const;
    bool            IsTileCoordsOutOfBounds(IntVec2 const& tileCoords) const;
    IntVec2         RollRandomTileCoords() const;
    IntVec2         RollRandomTraversableTileCoords(TileHeatMap const& heatMap, IntVec2 const& startCoords) const;

    // Heatmap-related
    void              GenerateHeatMaps(TileHeatMap const& heatMap) const;
    void              PopulateDistanceField(TileHeatMap const& heatMap, IntVec2 const& startCoords, float specialValue) const;
    void              PopulateDistanceFieldForEntity(TileHeatMap const& heatMap, IntVec2 const& startCoords, float specialValue) const;
    void              PopulateDistanceFieldForLandBased(TileHeatMap const& heatMap) const;
    void              PopulateDistanceFieldForAmphibian(TileHeatMap const& heatMap) const;
    void              PopulateDistanceFieldToPosition(TileHeatMap const& heatMap, IntVec2 const& playerCoords) const;
    std::vector<Vec2> GenerateEntityPathToGoal(TileHeatMap const& heatMap,Vec2 const& start, Vec2 const& goal) const;
    bool              RaycastHitsImpassable(Vec2 const& currentPos, Vec2 const& nextNextPos);

private:
    void UpdateEntities(float deltaSeconds) const;
    void RenderTiles() const;
    void RenderEntities() const;
    void RenderTileHeatMap() const;
    void DebugRenderEntities() const;
    void DebugRenderTileIndex() const;

    void InitializeTileHeatMaps();

// Map-related
    void GenerateAllTiles();
    void GenerateTilesByType(String const& tileName);
    void GenerateWormTiles(String const& wormTileName, int numWorms, int wormLength);
    void GenerateLShapeTiles(int tileCoordX, int tileCoordY, int width, int height, bool isBottomLeft);
    void GenerateStartPosTile();
    void GenerateExitPosTile();
    void SetTileAtCoords(String const& tileName, int tileX, int tileY);
    void ConvertUnreachableTilesToSolid(TileHeatMap const& heatMap, String const& tileName);
    bool IsEdgeTile(int x, int y) const;
    bool IsTileCoordsInLShape(int x, int y) const;
    bool IsWorldPosOccupied(Vec2 const& position) const;
    bool IsWorldPosOccupiedByEntity(Vec2 const& position, EntityType type) const;
    bool IsValidMap(IntVec2 const& startCoords, IntVec2 const& exitCoords, int maxAttempts);

    AABB2 const GetTileBounds(IntVec2 const& tileCoords) const;
    AABB2 const GetTileBounds(int tileIndex) const;
    IntVec2     RollRandomCardinalDirection() const;

    // Entity-lifetime-related
    Entity* CreateNewEntity(EntityType type, EntityFaction faction);
    void    AddEntityToList(Entity* entity, EntityList& entityList);
    void    RemoveEntityFromList(Entity const* entity, EntityList& entityList);
    void    DeleteGarbageEntities();
    void    SpawnNewNPCs();
    bool    IsBullet(Entity const* entity) const;
    bool    IsAgent(Entity const* entity) const;

    // Entity-physic-related
    void PushEntitiesOutOfWalls() const;
    void PushEntityOutOfSolidTiles(Entity* entity) const;
    void PushEntityOutOfTileIfSolid(Entity* entity, IntVec2 const& tileCoords) const;
    void PushEntitiesOutOfEachOther(EntityList const& entityListA, EntityList const& entityListB) const;
    void CheckEntityVsEntityCollision(EntityList const& entityListA, EntityList const& entityListB);

    std::vector<Tile>    m_tiles;
    EntityList           m_allEntities;
    EntityList           m_entitiesByType[NUM_ENTITY_TYPES];
    EntityList           m_agentsByFaction[NUM_ENTITY_FACTIONS];
    EntityList           m_bulletsByFaction[NUM_ENTITY_FACTIONS];
    IntVec2              m_startPosition = IntVec2::ZERO;
    IntVec2              m_exitPosition  = IntVec2::ZERO;
    IntVec2              m_dimensions;
    MapDefinition const* m_mapDef = nullptr;

    // MetaData management
    std::vector<TileHeatMap*> m_tileHeatMaps;
    Entity*                   m_currentSelectedEntity   = nullptr;
    int                       m_currentTileHeatMapIndex = -1;
};
