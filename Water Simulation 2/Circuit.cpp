#include "main.h"
#include "Circuit.h"
#include <math.h>

// Global array holding the world-space lamp (light bulb) position of each streetlight.
// Populated by RenderStreetlights() every frame.
StreetlightPos g_StreetlightLamps[NUM_STREETLIGHTS];

// ---- Circuit parameters ----
// Oval centered at (512, ?, 512) on the terrain
#define CIRCUIT_CX          512.0f
#define CIRCUIT_CZ          512.0f
#define CIRCUIT_RX_OUT      160.0f   // outer circle radius
#define CIRCUIT_RZ_OUT      160.0f   // outer circle radius (equal → perfect circle)
#define CIRCUIT_ROAD_WIDTH   40.0f   // width of road strip
#define CIRCUIT_SEGMENTS     64      // number of quad segments around oval
#define ROAD_Y_OFFSET         0.8f   // raise road slightly above terrain

#define PI2 (2.0f * 3.14159265f)

// Extern the fog coordinate function and heightmap
extern PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;

// Returns terrain Y at (x, z), hugging the actual terrain height.
static float CircuitY(float x, float z)
{
    return (float)Height(g_HeightMap, (int)x, (int)z) + ROAD_Y_OFFSET;
}

// Returns true if the terrain at (x, z) is safely above the water surface.
// The margin of 4 units prevents z-fighting and skips edge-segments that
// would partially dip into the water plane.
static bool AboveWater(float x, float z)
{
    return (float)Height(g_HeightMap, (int)x, (int)z) >= g_WaterHeight + 4.0f;
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

        // Skip any segment where any corner sits on or below the water surface
        if (!AboveWater(ox0, oz0) || !AboveWater(ox1, oz1) ||
            !AboveWater(ix0, iz0) || !AboveWater(ix1, iz1))
            continue;

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

    float x2 = x + w,  y2 = by + h,  z2 = z + d;

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);

    // ----- Walls -----
    glBindTexture(GL_TEXTURE_2D, wallTex);
    glBegin(GL_QUADS);

        // Front face (z2 side) — normal points +Z
        glNormal3f(0, 0, 1);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x,  by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x2, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x2, y2, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x,  y2, z2);

        // Back face (z side) — normal points -Z
        glNormal3f(0, 0, -1);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x2, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x,  by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x,  y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x2, y2, z);

        // Left face (x side) — normal points -X
        glNormal3f(-1, 0, 0);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x, y2, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x, y2, z);

        // Right face (x2 side) — normal points +X
        glNormal3f(1, 0, 0);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,0); glVertex3f(x2, by, z2);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,0); glVertex3f(x2, by, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(1,1); glVertex3f(x2, y2, z);
        glFogCoordfEXT(0.0f); glTexCoord2f(0,1); glVertex3f(x2, y2, z2);

    glEnd();

    // ----- Roof -----
    glBindTexture(GL_TEXTURE_2D, roofTex);
    glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
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
// Helper: DrawStreetlight
//   Pole (thin cylinder) + lamp arm + lamp head (small box).
//   Stores the lamp world-position into g_StreetlightLamps[idx].
// ---------------------------------------------------------------
static void DrawStreetlight(float x, float z, int idx)
{
    float gy = (float)Height(g_HeightMap, (int)x, (int)z);

    const float poleH  = STREETLIGHT_HEIGHT;  // height of pole
    const float poleR  = 0.6f;                // pole radius
    const float armLen = 5.0f;                // horizontal arm length
    const float headSz = 1.5f;               // lamp head half-size
    const int   SIDES  = 8;

    // Lamp-bulb world position (used as the light source)
    float lampX = x + armLen;
    float lampY = gy + poleH + headSz;
    float lampZ = z;

    // Store for rendering
    if (idx >= 0 && idx < NUM_STREETLIGHTS)
    {
        g_StreetlightLamps[idx].x = lampX;
        g_StreetlightLamps[idx].y = lampY;
        g_StreetlightLamps[idx].z = lampZ;
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    // ----- Pole (dark grey cylinder) -----
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= SIDES; i++)
    {
        float a  = (float)i / SIDES * PI2;
        float ca = cosf(a) * poleR;
        float sa = sinf(a) * poleR;
        float nx = cosf(a), nz = sinf(a);
        glNormal3f(nx, 0, nz);
        glFogCoordfEXT(0.0f);
        glVertex3f(x + ca, gy,          z + sa);
        glVertex3f(x + ca, gy + poleH,  z + sa);
    }
    glEnd();

    // ----- Horizontal arm -----
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= SIDES; i++)
    {
        float a  = (float)i / SIDES * PI2;
        float ca = cosf(a) * poleR;
        float sa = sinf(a) * poleR;
        // Arm goes along X, so normal is in the YZ plane
        glNormal3f(0, cosf(a), sinf(a));
        glFogCoordfEXT(0.0f);
        glVertex3f(x,          gy + poleH + ca, z + sa);
        glVertex3f(x + armLen, gy + poleH + ca, z + sa);
    }
    glEnd();

    // ----- Lamp head (bright yellow box) -----
    float hx  = lampX - headSz,  hx2 = lampX + headSz;
    float hy  = lampY - headSz,  hy2 = lampY + headSz;
    float hz  = lampZ - headSz,  hz2 = lampZ + headSz;

    glColor3f(1.0f, 1.0f, 0.4f);   // bright yellow emissive look

    // Set a high emission so the lamp always looks lit
    float emissive[4] = {1.0f, 1.0f, 0.4f, 1.0f};
    float noEmissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, emissive);

    glBegin(GL_QUADS);
        glNormal3f( 0, 0, 1); glFogCoordfEXT(0); glVertex3f(hx,hy,hz2); glVertex3f(hx2,hy,hz2); glVertex3f(hx2,hy2,hz2); glVertex3f(hx,hy2,hz2);
        glNormal3f( 0, 0,-1); glFogCoordfEXT(0); glVertex3f(hx2,hy,hz); glVertex3f(hx,hy,hz);   glVertex3f(hx,hy2,hz);   glVertex3f(hx2,hy2,hz);
        glNormal3f(-1, 0, 0); glFogCoordfEXT(0); glVertex3f(hx,hy,hz);  glVertex3f(hx,hy,hz2);  glVertex3f(hx,hy2,hz2);  glVertex3f(hx,hy2,hz);
        glNormal3f( 1, 0, 0); glFogCoordfEXT(0); glVertex3f(hx2,hy,hz2);glVertex3f(hx2,hy,hz);  glVertex3f(hx2,hy2,hz);  glVertex3f(hx2,hy2,hz2);
        glNormal3f( 0, 1, 0); glFogCoordfEXT(0); glVertex3f(hx,hy2,hz); glVertex3f(hx2,hy2,hz); glVertex3f(hx2,hy2,hz2); glVertex3f(hx,hy2,hz2);
        glNormal3f( 0,-1, 0); glFogCoordfEXT(0); glVertex3f(hx,hy,hz2); glVertex3f(hx2,hy,hz2); glVertex3f(hx2,hy,hz);   glVertex3f(hx,hy,hz);
    glEnd();

    glMaterialfv(GL_FRONT, GL_EMISSION, noEmissive);

    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);  // restore
}


// ---------------------------------------------------------------
// MakeShadowMatrix (internal helper)
//   Fills a 16-float column-major OpenGL matrix that projects
//   geometry onto plane (plane[0..3]) from light (light[0..3]).
// ---------------------------------------------------------------
static void MakeShadowMatrix(float plane[4], float light[4], float mat[16])
{
    float dot = plane[0]*light[0] + plane[1]*light[1]
              + plane[2]*light[2] + plane[3]*light[3];

    // Column 0
    mat[0]  = dot - light[0]*plane[0];
    mat[1]  =     - light[1]*plane[0];
    mat[2]  =     - light[2]*plane[0];
    mat[3]  =     - light[3]*plane[0];
    // Column 1
    mat[4]  =     - light[0]*plane[1];
    mat[5]  = dot - light[1]*plane[1];
    mat[6]  =     - light[2]*plane[1];
    mat[7]  =     - light[3]*plane[1];
    // Column 2
    mat[8]  =     - light[0]*plane[2];
    mat[9]  =     - light[1]*plane[2];
    mat[10] = dot - light[2]*plane[2];
    mat[11] =     - light[3]*plane[2];
    // Column 3
    mat[12] =     - light[0]*plane[3];
    mat[13] =     - light[1]*plane[3];
    mat[14] =     - light[2]*plane[3];
    mat[15] = dot - light[3]*plane[3];
}


// ---------------------------------------------------------------
// DrawBoxShadowGeom — geometry-only box for shadow pass
// ---------------------------------------------------------------
static void DrawBoxShadowGeom(float x, float z, float w, float h, float d)
{
    float by = (float)Height(g_HeightMap, (int)(x + w*0.5f), (int)(z + d*0.5f));
    float x2 = x + w,  y2 = by + h,  z2 = z + d;

    glBegin(GL_QUADS);
        glVertex3f(x, by,z2); glVertex3f(x2,by,z2); glVertex3f(x2,y2,z2); glVertex3f(x, y2,z2);
        glVertex3f(x2,by,z);  glVertex3f(x, by,z);  glVertex3f(x, y2,z);  glVertex3f(x2,y2,z);
        glVertex3f(x, by,z);  glVertex3f(x, by,z2); glVertex3f(x, y2,z2); glVertex3f(x, y2,z);
        glVertex3f(x2,by,z2); glVertex3f(x2,by,z);  glVertex3f(x2,y2,z);  glVertex3f(x2,y2,z2);
        glVertex3f(x, y2,z);  glVertex3f(x2,y2,z);  glVertex3f(x2,y2,z2); glVertex3f(x, y2,z2);
    glEnd();
}


// ---------------------------------------------------------------
// DrawTreeShadowGeom — geometry-only tree for shadow pass
// ---------------------------------------------------------------
static void DrawTreeShadowGeom(float x, float z, float trunkH, float trunkR,
                               float topH, float topR)
{
    float gy = (float)Height(g_HeightMap, (int)x, (int)z);

    const int SIDES = 8;

    // Trunk cylinder
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= SIDES; i++)
    {
        float a  = (float)i / SIDES * PI2;
        float ca = cosf(a) * trunkR, sa = sinf(a) * trunkR;
        glVertex3f(x+ca, gy,         z+sa);
        glVertex3f(x+ca, gy+trunkH,  z+sa);
    }
    glEnd();

    // Foliage cone
    float coneBase = gy + trunkH;
    float coneTop  = coneBase + topH;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < SIDES; i++)
    {
        float a0 = (float)i       / SIDES * PI2;
        float a1 = (float)(i + 1) / SIDES * PI2;
        glVertex3f(x+cosf(a0)*topR, coneBase, z+sinf(a0)*topR);
        glVertex3f(x+cosf(a1)*topR, coneBase, z+sinf(a1)*topR);
        glVertex3f(x, coneTop, z);
    }
    glEnd();
}


// ---------------------------------------------------------------
// RenderSceneShadows
//   Projects buildings + trees onto the plane y = groundY from
//   the given light position.  Call once per light source.
// ---------------------------------------------------------------
void RenderSceneShadows(float lightX, float lightY, float lightZ, float groundY)
{
    // Plane: y = groundY  -->  0*x + 1*y + 0*z + (-groundY) = 0
    float plane[4] = { 0.0f, 1.0f, 0.0f, -groundY };
    float light[4] = { lightX, lightY, lightZ, 1.0f };
    float shadowMat[16];
    MakeShadowMatrix(plane, light, shadowMat);

    // Save state
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    // Semi-transparent black shadow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);

    // Avoid z-fighting with the ground
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    glPushMatrix();
    glMultMatrixf(shadowMat);

    // --- Buildings ---
    DrawBoxShadowGeom(710, 465, 40, 60, 35);
    DrawBoxShadowGeom(710, 525, 35, 80, 30);
    DrawBoxShadowGeom(270, 465, 40, 55, 40);
    DrawBoxShadowGeom(278, 525, 30, 70, 35);
    DrawBoxShadowGeom(480, 310, 50, 65, 40);
    DrawBoxShadowGeom(490, 700, 45, 50, 38);

    // --- Trees ---
    DrawTreeShadowGeom(660, 500, 15.0f, 2.5f, 20.0f, 12.0f);
    DrawTreeShadowGeom(660, 540, 15.0f, 2.5f, 18.0f, 11.0f);
    DrawTreeShadowGeom(355, 500, 14.0f, 2.5f, 20.0f, 12.0f);
    DrawTreeShadowGeom(355, 540, 16.0f, 2.5f, 19.0f, 11.0f);
    DrawTreeShadowGeom(500, 358, 15.0f, 2.5f, 21.0f, 13.0f);
    DrawTreeShadowGeom(540, 358, 14.0f, 2.5f, 20.0f, 12.0f);
    DrawTreeShadowGeom(500, 665, 15.0f, 2.5f, 19.0f, 11.0f);
    DrawTreeShadowGeom(540, 665, 16.0f, 2.5f, 20.0f, 12.0f);

    glPopMatrix();

    // Restore state
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glColor4f(1,1,1,1);
}


// ---------------------------------------------------------------
// RenderStreetlights
//   Draws 6 streetlights around the circuit and updates
//   g_StreetlightLamps[] with each lamp's world position.
// ---------------------------------------------------------------
void RenderStreetlights()
{
    // Positions chosen outside the oval circuit (center 512,512, radii ~180x140)
    static const float sx[NUM_STREETLIGHTS] = { 700.0f, 700.0f, 320.0f, 320.0f, 515.0f, 515.0f };
    static const float sz[NUM_STREETLIGHTS] = { 480.0f, 550.0f, 480.0f, 550.0f, 300.0f, 730.0f };

    for (int i = 0; i < NUM_STREETLIGHTS; i++)
        DrawStreetlight(sx[i], sz[i], i);
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
