#ifndef _CIRCUIT_H
#define _CIRCUIT_H

// Texture IDs for circuit and static objects
#define ROAD_ID             50
#define BUILDING_WALL_ID    51
#define BUILDING_ROOF_ID    52
#define TREE_BARK_ID        53
#define TREE_LEAVES_ID      54

// Number of streetlights placed around the circuit
#define NUM_STREETLIGHTS    6

// Streetlight lamp height above terrain
#define STREETLIGHT_HEIGHT  30.0f

// Struct storing the world-space lamp position of each streetlight
struct StreetlightPos { float x, y, z; };

// Array of streetlight lamp positions (filled by RenderStreetlights)
extern StreetlightPos g_StreetlightLamps[NUM_STREETLIGHTS];

// Renders the oval street circuit with road texture
void RenderCircuit();

// Renders all static objects (6 buildings + 8 trees = 14 objects)
void RenderStaticObjects();

// Renders the streetlight poles and lamp heads
void RenderStreetlights();

// Renders planar shadows of buildings+trees for a given light source
// onto the horizontal plane y = groundY
void RenderSceneShadows(float lightX, float lightY, float lightZ, float groundY);

#endif
