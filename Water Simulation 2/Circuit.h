#ifndef _CIRCUIT_H
#define _CIRCUIT_H

// Texture IDs for circuit and static objects
#define ROAD_ID             50
#define BUILDING_WALL_ID    51
#define BUILDING_ROOF_ID    52
#define TREE_BARK_ID        53
#define TREE_LEAVES_ID      54

// Renders the oval street circuit with road texture
void RenderCircuit();

// Renders all static objects (6 buildings + 8 trees = 14 objects)
void RenderStaticObjects();

#endif
