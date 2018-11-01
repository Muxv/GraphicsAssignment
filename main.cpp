#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <model.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using std::vector;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(vector<std::string> faces);

void renderSkyBox(Shader &skyBoxShader);
void renderShip(Shader &shipShader, Model &shipModel, bool isRenderLight);
void renderWater(Shader &waterShader, Model &waterModel, bool isRenderLight);
void renderSun(Shader &sunShader, Model &sunModel);
void renderLight(Shader &objectShader, Model &objectModel);
void renderlightSpaceMatrix(Shader &objectShader);
void renderScreen();

bool DebugMode = false;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// settings
const unsigned int SCR_WIDTH = 1500;
const unsigned int SCR_HEIGHT = 1000;

// camera
glm::vec3 cameraOriginPlace = glm::vec3(0.0f, 0.0f, 3.0f);
Camera camera(cameraOriginPlace);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// depthMap
int depthResolution = 4;

// timing
double deltaTime = 0.0f;
double lastFrame = 0.0f;

//point light
glm::vec3 sunPos(0.829116, 2.7817, -4.29651);
//glm::vec3 sunPos(2.39247, 0.307163, -2.48315);
// ship pos
//glm::vec3 shipPos(0.0f, 3.0f, 0.0f);
glm::vec3 shipPos(0.0f, 0.0f, 0.0f);

int main()
{

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	// -----------------------------------------------------------------------------------

	vector <std::string> faces{
		"skybox/right.jpg",
		"skybox/left.jpg",
		"skybox/top.jpg",
		"skybox/bottom.jpg",
		"skybox/front.jpg",
		"skybox/back.jpg"
	};

	unsigned int cubemapTexture = loadCubemap(faces);

	// frameBuffer for color
	unsigned int hdrFBO;
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	// create 2 floating point color buffers (1 for normal rendering, other for brightness treshold values)
	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// framebuffer for blurring
	unsigned int pingpongFBO[2];
	unsigned int pingpongColorbuffers[2];
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
		// also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}

	// frameBuffer for depth
	const unsigned int SHADOW_WIDTH = 1024 * depthResolution;
	const unsigned int SHADOW_HEIGHT = 1024 * depthResolution;

	unsigned int depthFBO;
	glGenFramebuffers(1, &depthFBO);

	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// -----------------------------------------------------------------------------------

	Shader skyboxShader("shader/skybox.vs", "shader/skybox.fs");
	Shader objectShader("shader/object.vs", "shader/object.fs");
	Shader sunShader("shader/sun.vs", "shader/sun.fs");
	Shader depthMappingShader("shader/depth_map.vs", "shader/depth_map.fs");
	Shader debugDepthQuad("shader/debug.vs", "shader/debug.fs");
	Shader hdrShader("shader/hdr.vs", "shader/hdr.fs");
	Shader blurShader("shader/blur.vs", "shader/blur.fs");

	Model sun("sun/sun.obj");
	Model ourModel("boat/boat_new.obj");
	Model water("water/water.obj");

	debugDepthQuad.use();
	debugDepthQuad.setInt("depthMap", 0);

	objectShader.use();
	objectShader.setInt("shadowMap", 10);

	skyboxShader.use();
	skyboxShader.setInt("skyboxTexture", 0);

	blurShader.use();
	blurShader.setInt("image", 0);

	hdrShader.use();
	hdrShader.setInt("scene", 0);
	hdrShader.setInt("bloomBlur", 1);

	

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glEnable(GL_CULL_FACE);

	while (!glfwWindowShouldClose(window))
	{
		
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
	// first rendering --> depth
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			renderWater(depthMappingShader, water, false);
			renderShip(depthMappingShader, ourModel, false);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// second rendering --> color
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			renderSkyBox(skyboxShader);
			renderSun(sunShader, sun);
			glActiveTexture(GL_TEXTURE10);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			renderShip(objectShader, ourModel, true);
			renderWater(objectShader, water, true);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//  third rendering --->blur
		bool horizontal = true, first_iteration = true;
		unsigned int amount = 10;
		blurShader.use();
		for (unsigned int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			blurShader.setInt("horizontal", horizontal);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
			renderScreen();
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//  fourth rendering  --> the scene  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
		hdrShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
		hdrShader.setInt("bloom", bloom);
		hdrShader.setFloat("exposure", exposure);
		renderScreen();

		//std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;

	// use for debug
		if (DebugMode) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			float near_plane = 1.0f, far_plane = 7.5f;
			debugDepthQuad.use();
			debugDepthQuad.setFloat("near_plane", near_plane);
			debugDepthQuad.setFloat("far_plane", far_plane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
			renderScreen();
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << "\n";
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed)
	{
		bloom = !bloom;
		bloomKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		bloomKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		if (exposure > 0.0f)
			exposure -= 0.001f;
		else
			exposure = 0.0f;
	}
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		exposure += 0.001f;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void renderSkyBox(Shader &skyBoxShader) {

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	GLuint skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();
	view = glm::mat4(glm::mat3(view));

	skyBoxShader.use();
	skyBoxShader.setMat4("view", view);
	skyBoxShader.setMat4("projection", projection);

	glBindVertexArray(skyboxVAO);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void renderSun(Shader &sunShader, Model &sunModel) {

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, sunPos);
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	sunShader.use();
	sunShader.setMat4("model", model);
	sunShader.setMat4("projection", projection);
	sunShader.setMat4("view", view);
	sunModel.Draw(sunShader);
}

void renderWater(Shader &waterShader, Model &waterModel, bool isRenderLight) {

	waterShader.use();
	renderlightSpaceMatrix(waterShader);
	glm::mat4 model = glm::mat4(1.0f);
	waterShader.setMat4("model", model);

	if (!isRenderLight) { // for render depth

	}
	else if (isRenderLight) {
		waterShader.setInt("objectNum", 2);
		
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		waterShader.setMat4("projection", projection);
		waterShader.setMat4("view", view);
		renderLight(waterShader, waterModel);
	}
	waterModel.Draw(waterShader);
}

void renderShip(Shader &shipShader, Model &shipModel, bool isRenderLight) {
	
	shipShader.use();

	renderlightSpaceMatrix(shipShader);
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, shipPos);
	model = glm::scale(model, glm::vec3(0.001f, 0.001f, 0.001f));
	
	shipShader.setMat4("model", model);

	if (!isRenderLight) { // for render depth

	}
	else if (isRenderLight) {

		shipShader.setInt("objectNum", 1);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		shipShader.setMat4("projection", projection);
		shipShader.setMat4("view", view);
		renderLight(shipShader, shipModel);
	}
	shipModel.Draw(shipShader);

}

void renderlightSpaceMatrix(Shader &objectShader) {

	glm::mat4 lightProjection = glm::mat4(1.0f);
	glm::mat4 lightView = glm::mat4(1.0f);
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
	float near_plane = 1.0f, far_plane = 7.5f;
	lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(sunPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
}

void renderLight(Shader &objectShader, Model &objectModel) {

	// without position infomation
	objectShader.setVec3("viewPos", camera.Position);

	objectShader.setVec3("sunLight.position", sunPos);
	objectShader.setVec3("sunLight.ambient", 0.05f, 0.05f, 0.05f);
	objectShader.setVec3("sunLight.diffuse", 0.8f, 0.8f, 0.8f);
	objectShader.setVec3("sunLight.specular", 1.0f, 1.0f, 1.0f);

	objectShader.setVec3("rightLight.direction", shipPos - glm::vec3(2.04f, 0.72f, 2.35f));
	objectShader.setVec3("rightLight.ambient", 0.05f, 0.05f, 0.05f);
	objectShader.setVec3("rightLight.diffuse", 0.5f, 0.5f, 0.5f);
	objectShader.setVec3("rightLight.specular", 0.7f, 0.7f, 0.7f);

	objectShader.setVec3("leftLight.direction", shipPos - glm::vec3(3.36, 0.72, 0.224));
	objectShader.setVec3("leftLight.ambient", 0.05f, 0.05f, 0.05f);
	objectShader.setVec3("leftLight.diffuse", 0.5f, 0.5f, 0.5f);
	objectShader.setVec3("leftLight.specular", 0.7f, 0.7f, 0.7f);

	objectShader.setVec3("backLight.direction", shipPos - glm::vec3(-3.17, 0.77, -1.82));
	objectShader.setVec3("backLight.ambient", 0.05f, 0.05f, 0.05f);
	objectShader.setVec3("backLight.diffuse", 0.5f, 0.5f, 0.5f);
	objectShader.setVec3("backLight.specular", 0.7f, 0.7f, 0.7f);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderScreen()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
