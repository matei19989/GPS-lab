
// This is a compiler directive that includes libraries (For Visual Studio).
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#include "main.h"								// This includes our main header file

bool  g_bFullScreen = true;						// Set full screen as default
HWND  g_hWnd;									// This is the handle for the window
RECT  g_rRect;									// This holds the window dimensions
HDC   g_hDC;									// General HDC - (handle to device context)
HGLRC g_hRC;									// General OpenGL_DC - Our Rendering Context for OpenGL
HINSTANCE g_hInstance;							// This holds the global hInstance for UnregisterClass() in DeInit()

/*
CONTROLS:

 The camera is moved by the mouse and the arrow keys (also w, a, s, d).  
 The mouse buttons turn detail texturing on/off as well as switch to wireframe.  The fog in the water can be changed by the + and minus keys.  We also added these keys:
 F1 - Makes the water appear farther away by shrinking the water texture
 F2 - Makes the water appear closer by enlarging the water texture
 F3 - Makes the water move faster
 F4 - Makes the water slower

*/


// This is our global shader object that will handle our shaders
CShader g_Shader;

// The size of the textures that we will render on the fly (reflection/refraction/depth)
int g_TextureSize = 512;

// The height of the water that is above the terrain floor
float g_WaterHeight = 30.0f;

// The scale for the water textures
float g_WaterUV = 35.0f;

// The speed of the water flow
float g_WaterFlow = 0.002f;

// The scale for the caustics (light textures underwater)
const float kCausticScale = 4.0f;

// This is our fog extension function pointer to set a vertice's depth
PFNGLFOGCOORDFEXTPROC glFogCoordfEXT = NULL;

// This stores the desired depth that we want to fog
float g_FogDepth = 30.0f; // (* NEW *)

// Our function pointers for the ARB multitexturing functions
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB	 = NULL;
PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB		 = NULL;

// This controls if we have detail texturing on or off
bool g_bDetail = true;

// This handles the current scale for the texture matrix for the detail texture
int g_DetailScale = 16;

// This is our global camera object
CCamera g_Camera;								

// This holds the height map data
BYTE g_HeightMap[MAP_SIZE*MAP_SIZE];			

// This tells us if we want lines or fill mode
bool  g_bRenderMode = true;						

// This holds the texture info by an ID
UINT g_Texture[MAX_TEXTURES] = {0};		


///////////////////////////////// INIT \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
// This function initializes the application

void Init(HWND hWnd)
{
	g_hWnd = hWnd;										// Assign the window handle to a global window handle
	GetClientRect(g_hWnd, &g_rRect);					// Assign the windows rectangle to a global RECT
	InitializeOpenGL(g_rRect.right, g_rRect.bottom);	// Init OpenGL with the global rect

	// Here we initialize our multitexturing functions
	glActiveTextureARB		= (PFNGLACTIVETEXTUREARBPROC)		wglGetProcAddress("glActiveTextureARB");
	glMultiTexCoord2fARB	= (PFNGLMULTITEXCOORD2FARBPROC)		wglGetProcAddress("glMultiTexCoord2fARB");

	// Make sure our multi-texturing extensions were loaded correctly
	if(!glActiveTextureARB || !glMultiTexCoord2fARB)
	{
		// Print an error message and quit.
		MessageBox(g_hWnd, "Your current setup does not support multitexturing", "Error", MB_OK);
		PostQuitMessage(0);
	}

	// Find the correct function pointer that houses the fog coordinate function
	glFogCoordfEXT	= (PFNGLFOGCOORDFEXTPROC) wglGetProcAddress("glFogCoordfEXT");

	// Before trying to use this function pointer, we need to make sure it was
	// given a valid address.  If not, then we need to quit because something is wrong.
	if(!glFogCoordfEXT)
	{
		// Print an error message and quit.
		MessageBox(g_hWnd, "Your current setup does not support volumetric fog", "Error", MB_OK);
		PostQuitMessage(0);
	}

	// Pick a tan color for our fog with a full alpha
	float fogColor[4] = {0.2f, 0.2f, 0.9f, 1.0f};

	glEnable(GL_FOG);						// Turn on fog
	glFogi(GL_FOG_MODE, GL_LINEAR);			// Set the fog mode to LINEAR (Important)
	glFogfv(GL_FOG_COLOR, fogColor);		// Give OpenGL our fog color
	glFogf(GL_FOG_START, 0.0);				// Set the start position for the depth at 0
	glFogf(GL_FOG_END, 50.0);				// Set the end position for the depth at 50

	// Now we tell OpenGL that we are using our fog extension for per vertex fog
	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);

	// Here we read in the height map from the .raw file
	LoadRawFile("Terrain.raw", MAP_SIZE * MAP_SIZE, g_HeightMap);	

	glEnable(GL_DEPTH_TEST);							// Enables depth testing
	glEnable(GL_TEXTURE_2D);							// Enable texture mapping
	glEnable(GL_CULL_FACE);								// Turn on back-face culling

	// The extra initialization for this tutorial is to (1) position the camera
	// above the water, (2) create 3 textures for our reflection, refraction and
	// depth, (3) load all of our bitmap textures, including the caustic textures,
	// (4) as well as init and load our shaders.

	// Point our camera above the water, looking at the sun and reflected water
	g_Camera.PositionCamera( 475, 52, 301,  474, 52, 300,  0, 1, 0);

	// Since our water is reflecting the world that our camera is looking at,
	// we need to have 3 custom textures that we will render to.  These textures
	// will then be used in the shaders to do the reflection and refraction.
	// The depth texture will just store a single value for the depth of
	// each pixel.  This is why we use GL_DEPTH_COMPONENT and require 1 channel.
	CreateRenderTexture(g_Texture, g_TextureSize, 3, GL_RGB, REFLECTION_ID);
	CreateRenderTexture(g_Texture, g_TextureSize, 3, GL_RGB, REFRACTION_ID);
	CreateRenderTexture(g_Texture, g_TextureSize, 1, GL_DEPTH_COMPONENT, DEPTH_ID);
	
	CreateTexture(g_Texture[NORMAL_ID],	 "Textures\\normalmap.bmp");// Load the normal map water texture
	CreateTexture(g_Texture[DUDVMAP_ID], "Textures\\dudvmap.bmp");	// Load the dudv map water texture
	CreateTexture(g_Texture[TERRAIN_ID], "Textures\\Terrain.bmp");	// Load the terrain texture
	CreateTexture(g_Texture[DETAIL_ID],	 "Textures\\Detail.bmp");	// Load the detail texture for the terrain
	CreateTexture(g_Texture[BACK_ID],	 "Textures\\Back.bmp");		// Load the Sky box Back texture
	CreateTexture(g_Texture[FRONT_ID],	 "Textures\\Front.bmp");	// Load the Sky box Front texture
	CreateTexture(g_Texture[BOTTOM_ID],  "Textures\\Bottom.bmp");	// Load the Sky box Bottom texture
	CreateTexture(g_Texture[TOP_ID],	 "Textures\\Top.bmp");		// Load the Sky box Top texture
	CreateTexture(g_Texture[LEFT_ID],	 "Textures\\Left.bmp");		// Load the Sky box Left texture
	CreateTexture(g_Texture[RIGHT_ID],	 "Textures\\Right.bmp");	// Load the Sky box Right texture
	
	// Just like in our Win32 animation tutorial, load the 32 animated caustic 
	// textures starting with the prefix "caust".
	LoadAnimTextures(g_Texture, "Textures\\caust", WATER_START_ID, NUM_WATER_TEX);

	// Let's init our GLSL functions and make sure this computer can run the program.
	InitGLSL();

	// Here we pass in our vertex and fragment shader files to our shader object.
	g_Shader.InitShaders("water.vert", "water.frag");

	// Since we only use the shaders for the water we turn off the shaders by default.
	g_Shader.TurnOff();
}


///////////////////////////////// CREATE SKY BOX \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
// This creates a sky box centered around X Y Z with a width, height and length

void CreateSkyBox(float x, float y, float z, float width, float height, float length)
{
	// Turn on texture mapping if it's not already
	glEnable(GL_TEXTURE_2D);

	// Bind the BACK texture of the sky map to the BACK side of the cube
	glBindTexture(GL_TEXTURE_2D, g_Texture[BACK_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// This centers the sky box around (x, y, z)
	x = x - width  / 2;
	y = y - height / 2;
	z = z - length / 2;

	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);		
		
		// Assign the texture coordinates and vertices for the BACK Side
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x + width, y,			z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x + width, y + height, z); 
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x,			y + height, z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x,			y,			z);
		
	glEnd();

	// Bind the FRONT texture of the sky map to the FRONT side of the box
	glBindTexture(GL_TEXTURE_2D, g_Texture[FRONT_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);	
	
		// Assign the texture coordinates and vertices for the FRONT Side
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,			y,			z + length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,			y + height, z + length);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x + width, y + height, z + length); 
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x + width, y,			z + length);
	glEnd();

	// Bind the BOTTOM texture of the sky map to the BOTTOM side of the box
	glBindTexture(GL_TEXTURE_2D, g_Texture[BOTTOM_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);		
	
		// Assign the texture coordinates and vertices for the BOTTOM Side
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,			y,			z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,			y,			z + length);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x + width, y,			z + length); 
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x + width, y,			z);
	glEnd();

	// Bind the TOP texture of the sky map to the TOP side of the box
	glBindTexture(GL_TEXTURE_2D, g_Texture[TOP_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);		
		
		// Assign the texture coordinates and vertices for the TOP Side
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x + width, y + height, z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x + width, y + height, z + length); 
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,			y + height,	z + length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,			y + height,	z);
		
	glEnd();

	// Bind the LEFT texture of the sky map to the LEFT side of the box
	glBindTexture(GL_TEXTURE_2D, g_Texture[LEFT_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);		
		
		// Assign the texture coordinates and vertices for the LEFT Side
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,			y + height,	z);	
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x,			y + height,	z + length); 
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x,			y,			z + length);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,			y,			z);		
		
	glEnd();

	// Bind the RIGHT texture of the sky map to the RIGHT side of the box
	glBindTexture(GL_TEXTURE_2D, g_Texture[RIGHT_ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Start drawing the side as a QUAD
	glBegin(GL_QUADS);		

		// Assign the texture coordinates and vertices for the RIGHT Side
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x + width, y,			z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x + width, y,			z + length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x + width, y + height,	z + length); 
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x + width, y + height,	z);
	glEnd();
}

///////////////////////////////// RENDER WORLD \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
// This function renders the terrain and caustic effects

void RenderWorld(bool bRenderCaustics)
{
	// This function will be called a total of three times to
	// create our realistic water effect.  The first call is
	// render the world to our reflection texture.  The next
	// call is to render the world to our refraction and depth
	// texture.  The last call is to render the world normally.

	// This next function draws the terrain with the caustic animation.
	// A simple way we do this is to render the caustics all over
	// the terrain and then clip the terrain above the water.
	// This is the simplest way, however it is slower because we
	// have to render the terrain an extra time.  A better way would
	// be to use texturing in a shader that would slowly
	// fade out as it reached the water's surface.  This would also cut
	// out the hard break of the caustics when it reaches above water.
	// Projective textures are also an options.

	// I chose a scale of four for the caustic textures.  This would
	// depend on the water depth of course.  Adjust for realism in your world.
	// The water height is used for clipping the top part of the terrain.
	
	// Only render the caustics if we want to (i.e. not rendering reflection texture)
	if(bRenderCaustics)
		RenderCaustics(g_WaterHeight, kCausticScale);

	// Since we already are rendering the bottom part of the terrain with
	// caustics, we don't need to render the entire terrain again, just the
	// top part.  This is why we create a clipping plane reject all the
	// data below the water.  This could be done in the RenderHeightMap()
	// function so that we don't try and draw anything below the water in
	// this case, but like we said before, this is just a simple way to make
	// the effect happen and is not intended show a faster approach.  We would
	// also want to put the terrain in a display list if we wanted to increase
	// speed.

	// Anything above the water surface is okay to draw, but reject data below.
	// The first three values are the normal of the plane, and the last value
	// is the distance from the plane to clip.  Since this plane equation
	// is transformed by the inverse of the modelview matrix we have the distance
	// a negative number.  This can be confusing because it seems backwards.
	double TopTerrainPlane[4] = {0.0, 1.0, 0.0, -g_WaterHeight};
	
	// Turn a clipping plane on and assign our plane equation to it
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE0, TopTerrainPlane);

	// Render the clipped height map that is above the water
	RenderHeightMap(g_HeightMap);

	// Turn our clipping plane off
	glDisable(GL_CLIP_PLANE0);
	
	// Render our background sky box.
	CreateSkyBox(500, 500, 500, 1000, 1020, 1000);
}

///////////////////////////////// RENDER SCENE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
// This function renders the entire scene.

void RenderScene()
{
	// Before we start rendering the world at our set view port, we need to
	// render to our reflection, refraction and depth textures.  The view
	// port is set to our g_TextureSize and then render our textures.
	// Once that is finished, we need to reset the view port to our normal
	// window size.

	// Render the screen to a texture for our reflection, refraction and depth.
	// The texture size is for our view port, and the water height is for clipping.
	CreateReflectionTexture(g_WaterHeight, g_TextureSize);
    CreateRefractionDepthTexture(g_WaterHeight, g_TextureSize);

    // Restore the OpenGL view port to our original screen size
    glViewport(0,0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Clear our color and depth bits, as well as reset our current matrix.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

	// Get the current position of the camera
	CVector3 vPos		= g_Camera.Position();
	CVector3 vNewPos    = vPos;

	// * Basic Collision *
	// Check if the camera is below the height of the terrain at x and z,
	// but we add 10 to make it so the camera isn't on the floor.
	if(vPos.y < Height(g_HeightMap, (int)vPos.x, (int)vPos.z ) + 10)
	{
		// Set the new position of the camera so it's above the terrain + 10
		vNewPos.y = (float)Height(g_HeightMap, (int)vPos.x, (int)vPos.z ) + 10;

		// Get the difference of the y that the camera was pushed back up
		float temp = vNewPos.y - vPos.y;

		//  Get the current view and increase it by the different the position was moved
		CVector3 vView = g_Camera.View();
		vView.y += temp;

		// Set the new camera position.
		g_Camera.PositionCamera(vNewPos.x,  vNewPos.y,  vNewPos.z,
								vView.x,	vView.y,	vView.z,	0, 1, 0);								
	}

	// Set the current camera position and view
	g_Camera.Look();

	// Render the caustics, terrain and the sky box
    RenderWorld(true);

	// Now let's draw the water!  First we want to turn shaders on.
	// This will create the realistic effect.  Notice below that we
	// turn off culling for the water.  This is because we want to 
	// see both sides of the water.  When we go under we still want 
	// the water to be showing when we look up.  Keep in mind that 
	// the water is just a flat quad.  We could also just turn the
	// culling to FRONT when we are under the water, the reset it
	// to BACK after drawing the water.  This might increase speed.

	// Turn the shaders on for the water
	g_Shader.TurnOn();

	// Disable culling, draw the water at the g_WaterHeight, then turn
	// culling back on so we can see the water from above or below.
	glDisable(GL_CULL_FACE);
	RenderWater(g_WaterHeight);
	glEnable(GL_CULL_FACE);

	// Turn the shaders off again until we redraw the water
	g_Shader.TurnOff();

	// Swap the backbuffers to the foreground
	SwapBuffers(g_hDC);	
}


///////////////////////////////// MAIN LOOP \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
//  This function handles the main loop

WPARAM MainLoop()
{
	MSG msg;

	while(1)											// Do our infinite loop
	{													// Check if there was a message
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        { 
			if(msg.message == WM_QUIT)					// If the message wasn't to quit
				break;
            TranslateMessage(&msg);						// Find out what the message does
            DispatchMessage(&msg);						// Execute the message
        }
		else											// if there wasn't a message
		{ 		
			if(AnimateNextFrame(60))					// Make sure we only animate 60 FPS
			{
				g_Camera.Update();						// Update the camera data
				RenderScene();							// Render the scene every frame
			}
			else
			{
				Sleep(1);								// Let other processes work
			}
        } 
	}
	
	DeInit();											// Clean up and free all allocated memory

	return(msg.wParam);									// Return from the program
}


///////////////////////////////// WIN PROC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
//  This function handles the window messages.

LRESULT CALLBACK WinProc(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT    ps;

    switch (uMsg)
	{ 
    case WM_SIZE:										// If the window is resized
		if(!g_bFullScreen)								// Do this only if we are NOT in full screen
		{
			SizeOpenGLScreen(LOWORD(lParam),HIWORD(lParam));// LoWord=Width, HiWord=Height
			GetClientRect(hWnd, &g_rRect);				// Get the window rectangle
		}
        break; 
 
	case WM_PAINT:										// If we need to repaint the scene
		BeginPaint(hWnd, &ps);							// Init the paint struct		
		EndPaint(hWnd, &ps);							// EndPaint, Clean up
		break;


	case WM_LBUTTONDOWN:
	
		g_bDetail = !g_bDetail;
		break;

	case WM_RBUTTONDOWN:								// If the left mouse button was clicked
		
		g_bRenderMode = !g_bRenderMode;

		// Change the rendering mode to and from lines or triangles
		if(g_bRenderMode) 				
		{
			// Render the triangles in fill mode		
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
		}
		else 
		{
			// Render the triangles in wire frame mode
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	
		}
		break;

	case WM_KEYDOWN:
		switch(wParam) 
		{
			case VK_ESCAPE:								// Check if we hit the ESCAPE key.
				PostQuitMessage(0);						// Tell windows we want to quit
				break;

			case VK_SPACE:								// Check if we hit the SPACE bar

				// Times the current scale value by 2 and loop when it gets to 128
				g_DetailScale = (g_DetailScale * 2) % 128;

				// If the scale value is 0, set it to 1 again
				if(g_DetailScale == 0)
					g_DetailScale = 1;
				break;

			case VK_ADD:								// Check if we hit the + key

				g_FogDepth += 1;						// Increase the fog height

				if(g_FogDepth > 200)					// Make sure we don't go past 200
					g_FogDepth = 200;

				break;

			case VK_SUBTRACT:							// Check if we hit the - key

				g_FogDepth -= 1;						// Decrease the fog height

				if(g_FogDepth < 0)						// Make sure we don't go below 0
					g_FogDepth = 0;

				break;

			// To control the water's realism we added F1-F4 keys to change the
			// water's speed and texture size.  F1 is my favorite :)

			case VK_F1:									// Check if we hit the F1 key

				g_WaterUV += 1.0f;						// Increase the water UV texture
				break;
			
			case VK_F2:									// Check if we hit the F2 key

				g_WaterUV -= 1.0f;						// Increase the water UV texture

				if(g_WaterUV < 0.0f)					// If water's UV is below zero, reset it.
					g_WaterUV = 0.0f;
				break;

			case VK_F3:									// Check if we hit the F3 key

				g_WaterFlow += 0.00003f;				// Increase the water's speed
				break;

			
			case VK_F4:									// Check if we hit the F4 key

				g_WaterFlow -= 0.00003f;				// Decrease the water's speed

				if(g_WaterFlow < 0.0f)					// If speed is below zero, reset it.
					g_WaterFlow = 0.0f;
				break;

		}
		break;
 
    case WM_CLOSE:										// If the window is closed
        PostQuitMessage(0);								// Tell windows we want to quit
        break; 
    } 
 
    return DefWindowProc (hWnd, uMsg, wParam, lParam); 	// Return the default
}


