/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>
#include <chrono>
#include <algorithm>
#include <math.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"
#include "WaterBuffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;
	WaterBuffer * waterBuffer = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	//Our shader program for skybox
	std::shared_ptr<Program> cubeProg;

	std::shared_ptr<Program> waterProg;

	//our geometry
	vector<shared_ptr<Shape>> fox;
	shared_ptr<Shape> cube;
	vector<shared_ptr<Shape>> tree;
	shared_ptr<Shape> bridge;

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	//ground VAO
	GLuint GroundVertexArrayID;

	GLuint WaterBuffObj, WaterNorBuffObj, WaterTexBuffObj, WaterIndxBuffObj;
	//ground VAO
	GLuint WaterVertexArrayID;

	//the image to use as a texture (ground)
	shared_ptr<Texture> texture;
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;
	shared_ptr<Texture> texture2;
	shared_ptr<Texture> texture3;
	shared_ptr<Texture> bark_tex;
	shared_ptr<Texture> tree_tex;
	shared_ptr<Texture> grass_tex;
	shared_ptr<Texture> normal_map;
	shared_ptr<Texture> dudv_map;

	// global data for skybox
	unsigned int cubeMapTexture;
	// cube faces
	vector<std::string> faces{
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg"
	};

	//global data (larger program should be encapsulated)
	vec3 cameraEye = vec3(-20.0, 2.0, 30.0);
	vec3 cameraCenter = vec3(0.0, 0.0, 0.0);
	vec3 cameraUp = vec3(0.0, 1.0, 0.0);

	vec3 cameraEyeRecord;
	vec3 cameraCenterRecord;


	vec3 gMin;
	float gRot = 0;
	float gCamH = 0;
	float yaw = -90.0;
	float pitch = 0.0;
	float dollying = 0.0;
	float strafing = 0.0;
	float waveMovement = 0;
	bool right_hold = false;
	double prev_posX, prev_posY;
	bool goCamera = false;
	bool cinematic = false;
	Spline splinepath[2];
	//animation data
	float lightTrans = 0;
	float gTrans = -3;
	float sTheta = 0;
	float eTheta = 0;
	float hTheta = 0;

	// spatial grid
	bool spatialGrid[70][94] = { 0 };
	int xOffset = 35;
	int zOffset = 59;

	float camRot;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		//move camera left and right
		if (key == GLFW_KEY_A) {
			if (GLFW_PRESS == action) {
				strafing = -0.05f;
			}
			else if (GLFW_RELEASE == action) {
				strafing = 0.0f;
			}
		}
		if (key == GLFW_KEY_D) {
			if (GLFW_PRESS == action) {
				strafing = 0.05f;
			}
			else if (GLFW_RELEASE == action) {
				strafing = 0.0f;
			}
		}
		//move camera forward and backward
		if (key == GLFW_KEY_W) {
			if (GLFW_PRESS == action) {
				dollying = 0.05f;
			}
			else if (GLFW_RELEASE == action) {
				dollying = 0.0f;
			}
		}
		if (key == GLFW_KEY_S) {
			if (GLFW_PRESS == action) {
				dollying = -0.05f;
			}
			else if (GLFW_RELEASE == action) {
				dollying = 0.0f;
			}
		}
		// G key for entering and repeating cinematic mode
		if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
			if (!goCamera) {
				cameraEyeRecord = cameraEye;
				cameraCenterRecord = cameraCenter;
			}
			if (goCamera) {
				cameraEye = cameraEyeRecord;
				cameraCenter = cameraCenterRecord;
			}
			goCamera = !goCamera;
		}

		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans += 0.25;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans -= 0.25;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (GLFW_PRESS == action) {
				right_hold = true;
				glfwGetCursorPos(window, &prev_posX, &prev_posY);
			}
			else if (GLFW_RELEASE == action) {
				right_hold = false;
				//cout << "release: deltaX " << deltaX << " deltaY " << deltaY << endl;
			}
		}
	}

	void mousePositionCallback(GLFWwindow* window, double posX, double posY) {
		if (right_hold) {
			double posX, posY, deltaX, deltaY;
			glfwGetCursorPos(window, &posX, &posY);
			deltaX = prev_posX - posX;
			deltaY = posY - prev_posY;
			prev_posX = posX;
			prev_posY = posY;
			//cout << "hold: deltaX " << deltaX << " deltaY " << deltaY << endl;
			yaw -= deltaX * 0.3;
			pitch -= deltaY * 0.3;
			if (pitch > 89.0)
				pitch = 89.0;
			if (pitch < -89.0)
				pitch = -89.0;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		cout << "deltaX " << deltaX << " deltaY " << deltaY << endl;
		yaw += deltaX;
		pitch += deltaY;
		if (pitch > 89.0)
			pitch = 89.0;
		if (pitch < -89.0)
			pitch = -89.0;
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		//prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");
		prog->addUniform("Texture0");
		prog->addUniform("plane");
		prog->addUniform("colorMode");
		//prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		texProg->addUniform("plane");

		//read in a load the texture
		initTex(resourceDirectory);

		// Initialize the GLSL program that we will use for skybox
		cubeProg = make_shared<Program>();
		cubeProg->setVerbose(true);
		cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		cubeProg->init();
		cubeProg->addUniform("P");
		cubeProg->addUniform("V");
		cubeProg->addUniform("M");
		cubeProg->addAttribute("vertPos");
		cubeProg->addAttribute("vertNor");
		cubeProg->addAttribute("vertTex");

		// initialize the glsl program for water rendering
		waterProg = make_shared<Program>();
		waterProg->setVerbose(true);
		waterProg->setShaderNames(resourceDirectory + "/water_vert.glsl", resourceDirectory + "/water_frag.glsl");
		waterProg->init();
		waterProg->addUniform("reflectionTexture");
		waterProg->addUniform("refractionTexture");
		waterProg->addUniform("P");
		waterProg->addUniform("V");
		waterProg->addUniform("M");
		waterProg->addAttribute("vertPos");
		waterProg->addAttribute("vertNor");
		waterProg->addAttribute("vertTex");
		waterProg->addUniform("waveMovement");
		waterProg->addUniform("normalMap");
		waterProg->addUniform("lightPos");
		waterProg->addUniform("dudvMap");
		waterProg->addUniform("cameraPos");

		// init splines up and down
		splinepath[0] = Spline(glm::vec3(-20, 25, 25), glm::vec3(-1, 25, 25), glm::vec3(1, 25, 25), glm::vec3(20, 25, 25), 8);
		splinepath[1] = Spline(glm::vec3(0, 25, 30), glm::vec3(0, 25, 10), glm::vec3(0, 25, -10), glm::vec3(0, 25, -20), 8);

		initGrid();
	}

	// create a spatial grid of where the objects are
	void initGrid() {

		// top path
		for (int x = -27 + xOffset; x < 27 + xOffset; x++) {
			for (int z = -56 + zOffset; z < -39 + zOffset; z++) {
				spatialGrid[x][z] = 1;
			}
		}
		// remove area occupied by fox
		for (int x = -28 + xOffset; x < -20 + xOffset; x++) {
			for (int z = -51 + zOffset; z < -47 + zOffset; z++) {
				spatialGrid[x][z] = 0;
			}
		}
		// bridge path
		for (int x = 16 + xOffset; x < 24 + xOffset; x++) {
			for (int z = -40 + zOffset; z < 6 + zOffset; z++) {
				spatialGrid[x][z] = 1;
			}
		}
		// middle path; left of bridge
		for (int x = -32 + xOffset; x < 17 + xOffset; x++) {
			for (int z = -3 + zOffset; z < 14 + zOffset; z++) {
				spatialGrid[x][z] = 1;
			}
		}
		// middle path; right of bridge
		for (int x = 17 + xOffset; x < 33 + xOffset; x++) {
			for (int z = 5 + zOffset; z < 14 + zOffset; z++) {
				spatialGrid[x][z] = 1;
			}
		}
		// bottom path/ starting path
		for (int x = -24 + xOffset; x < -15 + xOffset; x++) {
			for (int z = 13 + zOffset; z < 33 + zOffset; z++) {
				spatialGrid[x][z] = 1;
			}
		}
	}

	void initTex(const std::string& resourceDirectory)
	{
		texture0 = make_shared<Texture>();
		texture0->setFilename(resourceDirectory + "/rock.jpg");
		texture0->init();
		texture0->setUnit(0);
		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture2 = make_shared<Texture>();
		texture2->setFilename(resourceDirectory + "/crate.jpg");
		texture2->init();
		texture2->setUnit(2);
		texture2->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture3 = make_shared<Texture>();
		texture3->setFilename(resourceDirectory + "/ground.jpg");
		texture3->init();
		texture3->setUnit(3);
		texture3->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		bark_tex = make_shared<Texture>();
		bark_tex->setFilename(resourceDirectory + "/bark.jpg");
		bark_tex->init();
		bark_tex->setUnit(4);
		bark_tex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		tree_tex = make_shared<Texture>();
		tree_tex->setFilename(resourceDirectory + "/tree_top.png");
		tree_tex->init();
		tree_tex->setUnit(5);
		tree_tex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		grass_tex = make_shared<Texture>();
		grass_tex->setFilename(resourceDirectory + "/august-steiro-grass-basecolor.jpg");
		grass_tex->init();
		grass_tex->setUnit(6);
		grass_tex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// texture unit for the water
		normal_map = make_shared<Texture>();
		normal_map->setFilename(resourceDirectory + "/normal.png");
		normal_map->init();
		normal_map->setUnit(3);
		normal_map->setWrapModes(GL_REPEAT, GL_REPEAT);

		dudv_map = make_shared<Texture>();
		dudv_map->setFilename(resourceDirectory + "/waterDUDV.png");
		dudv_map->init();
		dudv_map->setUnit(2);
		dudv_map->setWrapModes(GL_REPEAT, GL_REPEAT);


	}


	unsigned int createSky(string dir, vector<string> faces) {
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false);
		for (GLuint i = 0; i < faces.size(); i++) {
			unsigned char *data = stbi_load((dir + faces[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else {
				cout << "failed to load: " << (dir + faces[i]).c_str() << endl;
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		cout << " creating cube map any errors : " << glGetError() << endl;
		return textureID;
	}

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/fox.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			for (int i = 0; i < TOshapes.size(); i++) {
				fox.push_back(make_shared<Shape>());
				fox[i]->createShape(TOshapes[i]);
				fox[i]->measure();
				fox[i]->init();
			}
		}

		// Initialize cube mesh.
		vector<tinyobj::shape_t> TOshapesC;
		vector<tinyobj::material_t> objMaterialsC;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesC, objMaterialsC, errStr, (resourceDirectory + "/textured_cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {

			cube = make_shared<Shape>();
			cube->createShape(TOshapesC[0]);
			cube->measure();
			cube->init();
		}

		// Initialize tree mesh.
		vector<tinyobj::shape_t> TOshapesD;
		vector<tinyobj::material_t> objMaterialsD;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesD, objMaterialsD, errStr, (resourceDirectory + "/lowpolytree.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			for (int i = 0; i < TOshapesD.size(); i++) {
				tree.push_back(make_shared<Shape>());
				tree[i]->createShape(TOshapesD[i]);
				tree[i]->measure();
				tree[i]->init();
			}
		}

		vector<tinyobj::shape_t> TOshapesE;
		vector<tinyobj::material_t> objMaterialsE;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesE, objMaterialsE, errStr, (resourceDirectory + "/bridge.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			bridge = make_shared<Shape>();
			bridge->createShape(TOshapesE[0]);
			bridge->measure();
			bridge->init();
		}

		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();
		initWater();

		cubeMapTexture = createSky((resourceDirectory + "/anime-starry-night/"), faces);
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 100;
		float g_groundY = -0.25;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
     void drawGround(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	texture0->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
  		SetModel(vec3(0, -5, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

	 void initWater() {

		 float g_WaterSizeX = 80;
		 float g_WaterSizeZ = 18;
		 float g_WaterY = -3;

		 // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		 float WaterPos[] = {
			 -g_WaterSizeX, g_WaterY, -g_WaterSizeZ,
			 -g_WaterSizeX, g_WaterY,  g_WaterSizeZ,
			 g_WaterSizeX, g_WaterY,  g_WaterSizeZ,
			 g_WaterSizeX, g_WaterY, -g_WaterSizeZ
		 };

		 float WaterNorm[] = {
			 0, 1, 0,
			 0, 1, 0,
			 0, 1, 0,
			 0, 1, 0,
			 0, 1, 0,
			 0, 1, 0
		 };

		 static GLfloat WaterTex[] = {
			 0, 0, // back
			 0, 1,
			 1, 1,
			 1, 0 };

		 unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };

		 //generate the ground VAO
		 glGenVertexArrays(1, &WaterVertexArrayID);
		 glBindVertexArray(WaterVertexArrayID);

		 g_GiboLen = 6;
		 glGenBuffers(1, &WaterBuffObj);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterBuffObj);
		 glBufferData(GL_ARRAY_BUFFER, sizeof(WaterPos), WaterPos, GL_STATIC_DRAW);

		 glGenBuffers(1, &WaterNorBuffObj);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterNorBuffObj);
		 glBufferData(GL_ARRAY_BUFFER, sizeof(WaterNorm), WaterNorm, GL_STATIC_DRAW);

		 glGenBuffers(1, &WaterTexBuffObj);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterTexBuffObj);
		 glBufferData(GL_ARRAY_BUFFER, sizeof(WaterTex), WaterTex, GL_STATIC_DRAW);

		 glGenBuffers(1, &WaterIndxBuffObj);
		 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterIndxBuffObj);
		 glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
	 }

	 //code to draw the ground plane
	 void drawWater(shared_ptr<Program> curS) {
		 curS->bind();
		 glBindVertexArray(WaterVertexArrayID);
		 //draw the ground plane 
		 SetModel(vec3(0, 0, -22), 0, 0, 1, curS);
		 glEnableVertexAttribArray(0);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterBuffObj);
		 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		 glEnableVertexAttribArray(1);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterNorBuffObj);
		 glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		 glEnableVertexAttribArray(2);
		 glBindBuffer(GL_ARRAY_BUFFER, WaterTexBuffObj);
		 glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		 // draw!
		 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterIndxBuffObj);
		 glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

		 glDisableVertexAttribArray(0);
		 glDisableVertexAttribArray(1);
		 glDisableVertexAttribArray(2);
		 curS->unbind();
	 }

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

	void drawTree() {
		tree_tex->bind(prog->getUniform("Texture0"));
		tree[0]->draw(prog);
		tree_tex->unbind();
		bark_tex->bind(prog->getUniform("Texture0"));
		tree[1]->draw(prog);
		bark_tex->unbind();
	}

	void placeTrees(shared_ptr<MatrixStack> Model) {
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, -1.25, 10));

			//------------------ top half -----------------------------------------
			//last row path
			for (int i = 0; i < 10; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-40 + 10 * i, 9, -75));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(-45 + 10 * i, 9, -85));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}

			// left column 
			for (int i = 0; i < 3; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-40, 9, -70 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(-35, 9, -75 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}

			// right column 
			for (int i = 0; i < 3; i++) {
				Model->pushMatrix();
				Model->translate(vec3(40, 9, -70 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(35, 9, -75 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}
			// --------------------end of top half ----------------------------------

			// ----------center tree of the middle path----------------------------------
			// the left side
			for (int i = 0; i < 7; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-40, 9, -15 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(-45, 9, -10 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}
			// the right side
			for (int i = 0; i < 4; i++) {
				Model->pushMatrix();
				Model->translate(vec3(40, 9, -15 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(45, 9, -10 + i * 10));
				Model->scale(vec3(5, 5, 5));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}
			// ----------end of center tree of middle path--------------------------

			// right of the bridge
			Model->pushMatrix();
			Model->translate(vec3(30, 7, -10));
			Model->scale(vec3(4, 4, 4));
			setModel(prog, Model);
			drawTree();
			Model->popMatrix();

			// below the middle path
			for (int i = 0; i < 5; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-10 + 10 * i, 7, 10));
				Model->scale(vec3(4, 4, 4));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(-5 + 10 * i, 7, 20));
				Model->scale(vec3(4, 4, 4));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}

			// left of spawn
			for (int i = 0; i < 2; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-30, 7, 10 + 10 * i));
				Model->scale(vec3(4, 4, 4));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}

			//behind spawn
			for (int i = 0; i < 4; i++) {
				Model->pushMatrix();
				Model->translate(vec3(-30 + 10 * i, 7, 30));
				Model->scale(vec3(4, 4, 4));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
				Model->pushMatrix();
				Model->translate(vec3(-35 + 10 * i, 7, 35));
				Model->scale(vec3(4, 4, 4));
				setModel(prog, Model);
				drawTree();
				Model->popMatrix();
			}

		Model->popMatrix();
	}

	void drawGrass(shared_ptr<MatrixStack> Model) {
		Model->pushMatrix();
			Model->loadIdentity();
			Model->pushMatrix();
				Model->translate(vec3(0, -3, 45));
				Model->scale(vec3(150, 2, 100));
				setModel(prog, Model);
				cube->draw(prog);
			Model->popMatrix();
			Model->pushMatrix();
				Model->translate(vec3(0, -3, -64));
				Model->scale(vec3(150, 2, 50));
				setModel(prog, Model);
				cube->draw(prog);
			Model->popMatrix();
		Model->popMatrix();
	}

	void drawBridge(shared_ptr<MatrixStack> Model) {
		Model->pushMatrix();
			Model->loadIdentity();
			Model->pushMatrix();
				Model->translate(vec3(20, -10.5, -22));
				Model->scale(vec3(2, 1, 1.3));
				setModel(prog, Model);
				bridge->draw(prog);
			Model->popMatrix();
		Model->popMatrix();
	}

	void drawFox(shared_ptr<MatrixStack> Model) {
		Model->pushMatrix();
			Model->loadIdentity();
			Model->pushMatrix();
				Model->translate(vec3(-25, -1.5, -50));
				Model->scale(vec3(7, 7, 7));
				sTheta = sin(glfwGetTime()) * 0.4; 
				Model->rotate(sTheta + 0.3, vec3(0, 0, 1));
				setModel(prog, Model);
				for (int i = 0; i < fox.size(); i++) {
					SetMaterial(prog, i);
					fox[i]->draw(prog);
				}
			Model->popMatrix();
		Model->popMatrix();
	}

	void SetMaterial(shared_ptr<Program> curS, int i) {
		switch (i) {
		case 0: //shiny blue plastic
			//glUniform3f(curS->getUniform("MatAmb"), 0.24725, 0.1995, 0.0745);
			glUniform3f(curS->getUniform("MatDif"), 0.75164, 0.60648, 0.22648);
			glUniform3f(curS->getUniform("MatSpec"), 0.628281, 0.555802, 0.366065);
			glUniform1f(curS->getUniform("MatShine"), 51.2);
			break;
		case 1: // white rubber
			//glUniform3f(curS->getUniform("MatAmb"), 0.05, 0.05, 0.0);
			glUniform3f(curS->getUniform("MatDif"), 0.55, 0.55, 0.55);
			glUniform3f(curS->getUniform("MatSpec"), 0.70, 0.70, 0.70);
			glUniform1f(curS->getUniform("MatShine"), 32);
			break;
		}
	}

	void updateUsingCameraPath(float frametime) {
		if (goCamera) {
			if (!splinepath[0].isDone()) {
				splinepath[0].update(frametime);
				cameraEye = splinepath[0].getPosition();
			}
			else {
				splinepath[1].update(frametime);
				cameraEye = splinepath[1].getPosition();
			}
			if (splinepath[1].isDone()) {
				splinepath[0].reset();
				splinepath[1].reset();
			}
		}
	}

	void render(float frametime, vec4 plane, bool inverse) {

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 3000.0f);

		//create view matrix
		vec3 center;
		if (inverse) {
			center.x = cos(radians(yaw)) * cos(radians(-pitch));
			center.y = sin(radians(-pitch));
			center.z = sin(radians(yaw)) * cos(radians(-pitch));
		}
		else {
			center.x = cos(radians(yaw)) * cos(radians(pitch));
			center.y = sin(radians(pitch));
			center.z = sin(radians(yaw)) * cos(radians(pitch));
		}

		//dolly and strafing
		vec3 gaze = normalize(center);
		vec3 strafe = cross(gaze, cameraUp);
		gaze.y = 0;						// only move forward and backward; don't fly
		vec3 tempPos = cameraEye + dollying * gaze + strafing * strafe;
		if (spatialGrid[int(ceil(tempPos.x)) + xOffset][int(ceil(tempPos.z)) + zOffset]) {
			cameraEye = tempPos;
		}
		cameraCenter = normalize(center) + cameraEye;
		
		auto View = lookAt(cameraEye, cameraCenter, cameraUp);

		updateUsingCameraPath(frametime);

		// Draw the scene
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		glUniform3f(prog->getUniform("lightPos"), cameraEye.x, 3.0, cameraEye.z);
		glUniform4f(prog->getUniform("plane"), plane.x, plane.y, plane.z, plane.w);

		glUniform1i(prog->getUniform("colorMode"), 0);
		drawFox(Model);
		texture2->unbind();
		glUniform1i(prog->getUniform("colorMode"), 1);

		texture2->bind(prog->getUniform("Texture0"));
		drawBridge(Model);

		texture3->bind(prog->getUniform("Texture0"));
		texture3->unbind();
		placeTrees(Model);
		grass_tex->bind(prog->getUniform("Texture0"));
		drawGrass(Model);
		grass_tex->unbind();

		prog->unbind();

		//switch shaders to the texture mapping shader and draw the ground
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), cameraEye.x, 3.0f, cameraEye.z);	
		glUniform4f(texProg->getUniform("plane"), plane.x, plane.y, plane.z, plane.w);
		drawGround(texProg);
		texProg->unbind();

		//to draw the sky box bind the right shader
		cubeProg->bind();
		//set the projection matrix - can use the same one
		glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));

		//set the depth function to always draw the box
		glDepthFunc(GL_LEQUAL);
		//set up view matrix to include your view transforms
		glUniformMatrix4fv(cubeProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));

		//set and send model transforms - likely want a bigger cube
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(cameraEye);
			Model->scale(vec3(300, 100, 300));
			glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		Model->popMatrix();

		//bind the cube map texture
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		//draw the actual cube
		cube->draw(cubeProg);

		//set the depth test back to normal!
		glDepthFunc(GL_LESS);
		//unbind the shader for the skybox
		cubeProg->unbind();


		// Pop matrix stacks.
		Projection->popMatrix();

	}

	void renderWater() {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		////Use the matrix stack for Lab 6
		float aspect = width / (float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();
		auto View = lookAt(cameraEye, cameraCenter, cameraUp);
		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 3000.0f);

		// bind the water shader to draw water
		waterProg->bind();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUniform3f(waterProg->getUniform("cameraPos"), cameraEye.x, cameraEye.y, cameraEye.z);
		glUniform3f(waterProg->getUniform("lightPos"), cameraEye.x + 10.0f, 10.0f, cameraEye.z - 10.0f);

		// specify the location of the texture
		glUniform1i(waterProg->getUniform("reflectionTexture"), 0);
		glUniform1i(waterProg->getUniform("refractionTexture"), 1);
		glUniform1i(waterProg->getUniform("dudvMap"), 2);
		glUniform1i(waterProg->getUniform("normalMap"), 3);

		// bind the texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, waterBuffer->getReflectionTexture());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, waterBuffer->getRefractionTexture());
		dudv_map->bind(waterProg->getUniform("dudvMap"));
		normal_map->bind(waterProg->getUniform("normalMap"));

		// calculate how much the water will be moving
		waveMovement += 0.00005 * glfwGetTime();
		waveMovement = fmod(waveMovement, 1);
		glUniform1f(waterProg->getUniform("waveMovement"), waveMovement);

		//set the projection matrix - can use the same one
		glUniformMatrix4fv(waterProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		//set up view matrix to include your view transforms
		glUniformMatrix4fv(waterProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));

		drawWater(waterProg);
		//unbind the shader
		cubeProg->unbind();
		glDisable(GL_BLEND);
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1280, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;


	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	int width, height;
	glfwGetWindowSize(windowManager->getHandle(), &width, &height);
	WaterBuffer fbo(width, height);
	application->waterBuffer = &fbo;

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
			.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		glEnable(GL_CLIP_DISTANCE0);
		application->waterBuffer->bindReflectionFrameBuff();
		// make the water reflect the correct camera
		float distance = 2 * application->cameraEye.y + 4;
		application->cameraEye.y -= distance;

		application->render(deltaTime, vec4(0,1,0,3), true);
		application->cameraEye.y += distance;


		glfwGetWindowSize(windowManager->getHandle(), &width, &height);
		application->waterBuffer->unbindCurrentFrameBuff(width, height);

		application->waterBuffer->bindRefractionFrameBuff();
		application->render(deltaTime, vec4(0, -1, 0, -3), false);
		glDisable(GL_CLIP_DISTANCE0);

		glfwGetWindowSize(windowManager->getHandle(), &width, &height);
		application->waterBuffer->unbindCurrentFrameBuff(width, height);

		// Render scene.
		application->render(deltaTime, vec4(0,-1,0,-5), false);
		application->renderWater();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
