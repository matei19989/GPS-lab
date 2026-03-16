#include "main.h"
#include "Circuit.h"
#include <math.h>

// ---- Circuit parameters ----
// Oval centered at (512, ?, 512) on the terrain
#define CIRCUIT_CX          512.0f
#define CIRCUIT_CZ          512.0f
#define CIRCUIT_RX_OUT      180.0f   // outer oval X semi-axis
#define CIRCUIT_RZ_OUT      140.0f   // outer oval Z semi-axis
#define CIRCUIT_ROAD_WIDTH   40.0f   // width of road strip
#define CIRCUIT_SEGMENTS     64      // number of quad segments around oval
#define ROAD_Y_OFFSET         0.8f   // raise road slightly above terrain

#define PI2 (2.0f * 3.14159265f)

// Extern the fog coordinate function and heightmap
extern PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;

// Returns terrain Y at (x, z) with a minimum above water
static float CircuitY(float x, float z)
{
    float h = (float)Height(g_HeightMap, (int)x, (int)z);
    // Keep road above water (g_WaterHeight = 30)
    if (h < g_WaterHeight + 2.0f)
        h = g_WaterHeight + 2.0f;
    return h + ROAD_Y_OFFSET;
}

// ---------------------------------------------------------------
// RenderCircuit
//   Draws an oval racing circuit as a ring of textured quads.
//   The road follows the terrain height so it sits naturally.
// ---------------------------------------------------------------
void RenderCircuit()
{
    // Make sure we're on texture unit 0
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_Texture[ROAD_ID]);

    float cx = CIRCUIT_CX, cz = CIRCUIT_CZ;
    float rx_out = CIRCUIT_RX_OUT,  rz_out = CIRCUIT_RZ_OUT;
    float rx_in  = rx_out - CIRCUIT_ROAD_WIDTH;
    float rz_in  = rz_out - CIRCUIT_ROAD_WIDTH;

    for (int i = 0; i < CIRCUIT_SEGMENTS; i++)
    {
        float t0 = (float)i       / CIRCUIT_SEGMENTS * PI2;
        float t1 = (float)(i + 1) / CIRCUIT_SEGMENTS * PI2;

        // Outer ring vertices
        float ox0 = cx + rx_out * cosf(t0),  oz0 = cz + rz_out * sinf(t0);
        float ox1 = cx + rx_out * cosf(t1),  oz1 = cz + rz_out * sinf(t1);
        // Inner ring vertices
        float ix0 = cx + rx_in  * cosf(t0),  iz0 = cz + rz_in  * sinf(t0);
        float ix1 = cx + rx_in  * cosf(t1),  iz1 = cz + rz_in  * sinf(t1);

        // Heights
        float oy0 = CircuitY(ox0, oz0),  oy1 = CircuitY(ox1, oz1);
        float iy0 = CircuitY(ix0, iz0),  iy1 = CircuitY(ix1, iz1);

        // Texture V coordinate repeats 8x around the track (tiling road markings)
        float v0 = (float)i       / CIRCUIT_SEGMENTS * 8.0f;
        float v1 = (float)(i + 1) / CIRCUIT_SEGMENTS * 8.0f;

        glBegin(GL_QUADS);
            // inner-0
            glFogCoordfEXT(0.0f);
            glTexCoord2f(0.0f, v0); glVertex3f(ix0, iy0, iz0);
            // inner-1
            glFogCoordfEXT(0.0f);
            glTexCoord2f(0.0f, v1); glVertex3f(ix1, iy1, iz1);
            // outer-1
            glFogCoordfEXT(0.0f);
            glTexCoord2f(1.0f, v1); glVertex3f(ox1, oy1, oz1);
            // outer-0
            glFogCoordfEXT(0.0f);
            glTexCoord2f(1.0f, v0); glVertex3f(ox0, oy0, oz0);
        glEnd();
    }
}

// ---------------------------------------------------------------
// Helper: DrawBox
//   Renders a textured box (building).  x,z = base corner,
//   w/h/d = width/height/depth.  Y is sampled from terrain.
// ---------------------------------------------------------------
static void DrawBox(float x, float z, float w, float h, float d,
                    UINT wallTex, UINT roofTex)
{
    float by = (float)Height(g_HeightMap, (int)(x + w * 0.5f), (int)(z + d * 0.5f));
    if (by < g_WaterHeight + 2.0f)
        by = g_WaterHeight + 2.0f;

    float x2 = x + w,  y2 = by + h,  z2 = z + d;

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);

    // ----- Walls -----
    glBindTexture(GL_TEXTURE_2D, wallTex);
    glBegin(GL_QUADS);

        // Front face (z2 side)
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x,  by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x2, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x2, y2, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x,  y2, z2);

        // Back face (z side)
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x2, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x,  by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x,  y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x2, y2, z);

        // Left face (x side)
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x, y2, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x, y2, z);

        // Right face (x2 side)
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x2, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x2, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x2, y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x2, y2, z2);

    glEnd();

    // ----- Roof -----
    glBindTexture(GL_TEXTURE_2D, roofTex);
    glBegin(GL_QUADS);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x,  y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x2, y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x2, y2, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x,  y2, z2);
    glEnd();
}

// ---------------------------------------------------------------
// Helper: DrawTree
//   Renders a tree: cylinder trunk + cone foliage.
// ---------------------------------------------------------------
static void DrawTree(float x, float z, float trunkH, float trunkR,
                     float topH, float topR)
{
    float gy = (float)Height(g_HeightMap, (int)x, (int)z);
    if (gy < g_WaterHeight + 2.0f)
        gy = g_WaterHeight + 2.0f;

    const int SIDES = 8;

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);

    // ----- Trunk (cylinder as QUAD_STRIP) -----
    glBindTexture(GL_TEXTURE_2D, g_Texture[TREE_BARK_ID]);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= SIDES; i++)
    {
        float a  = (float)i / SIDES * PI2;
        float ca = cosf(a) * trunkR;
        float sa = sinf(a) * trunkR;
        float tx = (float)i / SIDES;
        glFogCoordfEXT(0.0f); glTexCoord2f(tx, 0.0f); glVertex3f(x + ca, gy,           z + sa);
        glFogCoordfEXT(0.0f); glTexCoord2f(tx, 1.0f); glVertex3f(x + ca, gy + trunkH,  z + sa);
    }
    glEnd();

    // ----- Foliage (cone) -----
    glBindTexture(GL_TEXTURE_2D, g_Texture[TREE_LEAVES_ID]);
    float coneBase = gy + trunkH;
    float coneTop  = coneBase + topH;

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < SIDES; i++)
    {
        float a0 = (float)i       / SIDES * PI2;
        float a1 = (float)(i + 1) / SIDES * PI2;

        glFogCoordfEXT(0.0f);
        glTexCoord2f((float)i / SIDES, 0.0f);
        glVertex3f(x + cosf(a0) * topR, coneBase, z + sinf(a0) * topR);

        glFogCoordfEXT(0.0f);
        glTexCoord2f((float)(i + 1) / SIDES, 0.0f);
        glVertex3f(x + cosf(a1) * topR, coneBase, z + sinf(a1) * topR);

        glFogCoordfEXT(0.0f);
        glTexCoord2f(0.5f, 1.0f);
        glVertex3f(x, coneTop, z);
    }
    glEnd();
}

// ---------------------------------------------------------------
// RenderStaticObjects
//   6 buildings + 8 trees placed around the outside of the circuit.
//   Total: 14 static objects (requirement: >= 10).
// ---------------------------------------------------------------
void RenderStaticObjects()
{
    UINT wallTex = g_Texture[BUILDING_WALL_ID];
    UINT roofTex = g_Texture[BUILDING_ROOF_ID];

    // ---- Buildings (6) ----
    // Positioned outside the oval circuit (outer radii ~180x140)
    // Circuit center (512, 512); buildings at ~radius 220+ from center

    // Right side cluster
    DrawBox(710, 465,  40, 60, 35, wallTex, roofTex);  // B1
    DrawBox(710, 525,  35, 80, 30, wallTex, roofTex);  // B2

    // Left side cluster
    DrawBox(270, 465,  40, 55, 40, wallTex, roofTex);  // B3
    DrawBox(278, 525,  30, 70, 35, wallTex, roofTex);  // B4

    // Top cluster
    DrawBox(480, 310,  50, 65, 40, wallTex, roofTex);  // B5

    // Bottom cluster
    DrawBox(490, 700,  45, 50, 38, wallTex, roofTex);  // B6

    // ---- Trees (8) ----
    // trunkH=15, trunkR=2.5, topH=20, topR=12
    DrawTree(660, 500,  15.0f, 2.5f, 20.0f, 12.0f);   // T1
    DrawTree(660, 540,  15.0f, 2.5f, 18.0f, 11.0f);   // T2
    DrawTree(355, 500,  14.0f, 2.5f, 20.0f, 12.0f);   // T3
    DrawTree(355, 540,  16.0f, 2.5f, 19.0f, 11.0f);   // T4
    DrawTree(500, 358,  15.0f, 2.5f, 21.0f, 13.0f);   // T5
    DrawTree(540, 358,  14.0f, 2.5f, 20.0f, 12.0f);   // T6
    DrawTree(500, 665,  15.0f, 2.5f, 19.0f, 11.0f);   // T7
    DrawTree(540, 665,  16.0f, 2.5f, 20.0f, 12.0f);   // T8
}
