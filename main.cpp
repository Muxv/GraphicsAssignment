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
void renderShip(Shader &shipShader, Model &shipModel, int renderTimes);
void renderWater(Shader &waterShader, Model &waterModel, int renderTimes);
void renderSun(Shader &sunShader, Model &sunModel);
void renderLight(Shader &objectShader, Model &objectModel);
void renderlightSpaceMatrix(Shader &objectShader, Model &objectModel);

// settings
const unsigned int SCR_WIDTH = 1500;
const unsigned int SCR_HEIGHT = 1000;

// camera
glm::vec3 cameraOriginPlace = glm::vec3(0.0f, 0.0f, 3.0f);
Camera camera(cameraOriginPlace);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//point light
glm::vec3 sunPos(-2.09321, 1.4637, -3.66447);
// ship pos
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
	// -----------------------------------------------------------------------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
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
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// -----------------------------------------------------------------------------------

	Shader skyboxShader("shader/skybox.vs", "shader/skybox.fs");
	Shader objectShader("shader/object.vs", "shader/object.fs");
	Shader sunShader("shader/sun.vs", "shader/sun.fs");
	Shader depthMappingShader("shader/depth_map.vs", "shader/depth_map.fs");

	Model sun("sun/sun.obj");
	Model ourModel("boat/boat_new.obj");
	Model water("water/water.obj");

	depthMappingShader.setInt("shadowMap", 0);
	objectShader.setInt("shadowMap", 0);
	skyboxShader.setInt("skyboxTexture", 1);
	

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

	// the first rendering
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			renderShip(depthMappingShader, ourModel, 1);
			renderWater(depthMappingShader, water, 1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// the second rendering 
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDepthFunc(GL_LEQUAL);
		renderSkyBox(skyboxShader);
		renderSun(sunShader, sun);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
			renderShip(objectShader, ourModel, 2);
			renderWater(objectShader, water, 2);
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
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << "\n";

	}
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

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

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
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	sunShader.use();

	sunShader.setMat4("model", glm::translate(model, sunPos));
	sunShader.setMat4("projection", projection);
	sunShader.setMat4("view", view);
	sunModel.Draw(sunShader);
}

void renderWater(Shader &waterShader, Model &waterModel, int renderTimes) {

	waterShader.use();
	renderlightSpaceMatrix(waterShader, waterModel);
	glm::mat4 model = glm::mat4(1.0f);
	waterShader.setMat4("model", model);


	if (renderTimes == 1) { // for render depth

	}
	else if (renderTimes == 2) {
		waterShader.setInt("objectNum", 2);
		
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		waterShader.setMat4("projection", projection);
		waterShader.setMat4("view", view);
		renderLight(waterShader, waterModel);
	}
	waterModel.Draw(waterShader);
}

void renderShip(Shader &shipShader, Model &shipModel, int renderTimes) {
	
	shipShader.use();

	renderlightSpaceMatrix(shipShader, shipModel);
	

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, shipPos);
	model = glm::scale(model, glm::vec3(0.001f, 0.001f, 0.001f));

	shipShader.setMat4("model", model);

	if (renderTimes == 1) { // for render depth

	}
	else if (renderTimes == 2) { // render normally
		shipShader.setInt("objectNum", 1);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		shipShader.setMat4("projection", projection);
		shipShader.setMat4("view", view);
		renderLight(shipShader, shipModel);
	}
	shipModel.Draw(shipShader);

}

void renderlightSpaceMatrix(Shader &objectShader, Model &objectModel) {

	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 0.25f, far_plane = 12.5f;
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

