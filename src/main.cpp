#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <cmath>
#include <vector>
#include <limits>

#include "Camera.hpp"
#include "Model.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static GLenum getError(const char* file, int line)
{
    GLenum errorCode;

    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;

        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        std::cout << error << " from error check at " << file << ": " << line;
    }

    return errorCode;
}
#define glCheckError() getError(__FILE__, __LINE__) 

static void errorCallbackGLFW(int code, const char* description)
{
    std::cout << "Error from GLFW from " << __FILE__ << " at line " << __LINE__ << ": " << description << std::endl;
}

Camera camera {glm::vec3(0.0f, 0.0f, 3.0f)};
float deltaTime = 0.0f;
static void handleInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float speed = camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += camera.forward * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= camera.forward * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= camera.right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += camera.right * speed;
}

// Global vars because parameters have to comply to callback
float lastX = SCR_WIDTH/2, lastY = SCR_HEIGHT/2;
static void mouseCallback(GLFWwindow* window, double x, double y)
{
    float xOffset = x - lastX;
    float yOffset = y - lastY;
    lastX = x;
    lastY = y;

    const float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;
    camera.yaw += xOffset;
    camera.pitch -= yOffset; // Rotating opposite direction for up and down look

    if (camera.pitch > 89.0f)
        camera.pitch = 89.0f;
    else if (camera.pitch < -89.0f)
        camera.pitch = -89.0f;

    glm::vec3 forward;
    float pitch = glm::radians(camera.pitch); 
    float yaw = glm::radians(camera.yaw);
    forward.x = cos(yaw) * cos(pitch);
    forward.y = sin(pitch);
    forward.z = sin(yaw) * cos(pitch);
    camera.forward = glm::normalize(forward);
    camera.right = glm::normalize(glm::cross(camera.forward, camera.up));
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) 
{
    glViewport(0, 0, width, height);
}

static const std::string parseShader(const char* filePath)
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::stringstream ss;

    try
    {
        shaderFile.open(filePath);
        ss << shaderFile.rdbuf();
        shaderFile.close();
    }
    catch(const std::ifstream::failure& e)
    {
        std::cerr << "FAILURE READING SHADER FILE: " << e.what() << '\n';
        return nullptr;
    }
    
    return ss.str();
}

static unsigned int compileShader(const unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        glCheckError();
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        
        std::cout << "Failed to compile shader: " << message << std::endl;
        glDeleteShader(id);

        return 0;
    }

    return id;
}

static unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    int result;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        glCheckError();
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(program, length, &length, message);

        std::cout << "Failed to link shader: " << message << std::endl;
        glDeleteProgram(program);

        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    // Detach shaders after linking and before deleting for final build

    return program;
}

static unsigned int loadTexture(const char* rootPath, const char* fileName)
{
    std::string path = std::string(rootPath) + std::string(fileName);
    unsigned int texture;
    glGenTextures(1, &texture);

	// Bind and set texture properties
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load texture image
	int width, height, channels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
	}

	stbi_image_free(data);

    return texture;
}

static Mesh processMesh(const char* rootPath, const aiMesh* const mesh, const aiScene* const scene, std::vector<Texture>& loadedTextures)
{
    unsigned int numVertices = mesh->mNumVertices;
    std::vector<Vertex> vertices;
    std::vector<Texture> textures;
    std::vector<unsigned int> indices;

    // Process vertex positions and normals
    for (unsigned int i = 0; i < numVertices; ++i)
    {
        const aiVector3D vertex = mesh->mVertices[i];
        Vertex newVertex;
        newVertex.position = glm::vec3(vertex.x, vertex.y, vertex.z);
        const aiVector3D normal = mesh->mNormals[i];
        newVertex.normal = glm::vec3(normal.x, normal.y, normal.z);
        vertices.push_back(newVertex);
    }

    // Process faces and their indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    // Process texture coordinates
    if (mesh->mTextureCoords[0] != nullptr)
    {
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            const aiVector3D texCoords = mesh->mTextureCoords[0][i];
            vertices[i].texCoords = glm::vec2(texCoords.x, texCoords.y);
        }
    }

    // Process materials and their textures
    unsigned int numSpecular = 1;
    unsigned int numDiffuse = 1;

    if (mesh->mMaterialIndex >= 0)
    {
        const aiMaterial* const material = scene->mMaterials[mesh->mMaterialIndex];

        for (unsigned int isSpecular = 0; isSpecular < 2; ++isSpecular)
        {
            bool specular = (bool)isSpecular;
            const aiTextureType type = specular ? aiTextureType_SPECULAR : aiTextureType_DIFFUSE;

            for (unsigned int i = 0; i < material->GetTextureCount(type); ++i)
            {
                aiString path;
				material->GetTexture(type, i, &path);
                const char* cstr = path.C_Str();
                bool loaded = false;
                Texture texture;
                
                for (unsigned int j = 0; j < loadedTextures.size(); ++j)
                {
                    if (std::strcmp(cstr, loadedTextures[j].file) == 0)
                    {
                        loaded = true;
                        texture = loadedTextures[j];
                        break;
                    }
                }

				std::string uniform = "texture_";

				if (specular)
				{
					uniform += "specular" + std::to_string(numSpecular);
					numSpecular++;
				}
				else
				{
					uniform += "diffuse" + std::to_string(numDiffuse);
					numDiffuse++;
				}
                    
                if (!loaded)
                {
				    texture = Texture(loadTexture(rootPath, cstr), specular, cstr, uniform.c_str());
                    loadedTextures.push_back(texture);
                }
                else
                {
                    texture.uniform = uniform.c_str();
                }

				textures.push_back(texture);
            }
        }
    }

    return Mesh(vertices, indices, textures);
}

static std::vector<Mesh> processNode(const char* rootPath, const aiNode* const node, const aiScene* const scene, std::vector<Texture>& loadedTextures)
{
    std::vector<Mesh> result; 

    // Process meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* const mesh = scene->mMeshes[node->mMeshes[i]];
        result.push_back(processMesh(rootPath, mesh, scene, loadedTextures));
    }

    // Recursively process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        std::vector<Mesh> nextMeshes = processNode(rootPath, node->mChildren[i], scene, loadedTextures);
        result.insert(result.end(), nextMeshes.begin(), nextMeshes.end());
    }

    return result;
}

static Model loadModel(const char* rootPath, const char* fileName)
{
	Assimp::Importer importer;
    std::string path = std::string(rootPath) + std::string(fileName);
    const aiScene* const scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (scene == nullptr || scene->mFlags != NULL && AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
    {
        std::cout << "Error loading model: " << importer.GetErrorString() << std::endl;
        return Model(std::vector<Mesh>{});
    }

    std::vector<Texture> loadedTextures; // Possibly change to set
    return Model(processNode(rootPath, scene->mRootNode, scene, loadedTextures));
}

int main()
{
    glfwInit();
    glfwSetErrorCallback(errorCallbackGLFW);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "COMPSCI 114 Final Project", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Successfully loaded OpenGL version " << glGetString(GL_VERSION) << " function pointers" << std::endl;
    }

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    // Set up camera and matrices
    float rotationAmount = 0.0f; float rotation = 0.0f; // For rotating model over time
    float fov = 45.0f; float near = 0.1f; float far = 100.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);

    // Load shaders
    const std::string vertexSource = parseShader("res/shaders/shader.vs");
    const std::string fragmentSource = parseShader("res/shaders/shader.fs");
    unsigned int shader = createShader(vertexSource, fragmentSource);
    glUseProgram(shader);

    // Load models
    const unsigned int modelCount = 1;
    Model models[modelCount][levelsOfDetail] = { { loadModel("res/backpack/backpack0/", "backpack.obj"), loadModel("res/backpack/backpack1/", "backpack.obj") } };
    glm::vec3 modelPositions[modelCount] = { glm::vec3(0.0f) };
    unsigned int modelLODs[modelCount]{ 0 };

    // Define distance thresholds for levels of detail
    const float nextThresholdMultiplier = 2.0f;
    float currentThreshold = 10.0f;
    float distanceThresholds[levelsOfDetail];

    for (unsigned int i = 0; i < levelsOfDetail; ++i)
    {
        distanceThresholds[i] = currentThreshold + (currentThreshold * nextThresholdMultiplier * i);
    }

    // Find uniform locations to send matrices to shaders later
    int modelLocation = glGetUniformLocation(shader, "model");
    int viewLocation = glGetUniformLocation(shader, "view");
    int projectionLocation = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) 
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        std::cout << "Current frame duration: " << std::to_string(deltaTime) << '\r' << std::flush;
        // std::cout << "FPS: " << std::to_string(1.0f / currentFrame) << '\r' << std::flush;

        handleInput(window);

        // After camera movement, find if we should increase or decrease level of detail by comparing the distance for each model
        for (unsigned int i = 0; i < modelCount; ++i)
        {
            float dist = glm::distance(camera.position, modelPositions[i]);

            for (unsigned int level = 0; level < levelsOfDetail; ++level)
            {
                if (level < (levelsOfDetail - 1) && dist > distanceThresholds[level])
                {
                    modelLODs[i] = level + 1; // Goes down a level of detail
                    break;
                }
                else if (level > 0 && dist < distanceThresholds[level])
                {
                    modelLODs[i] = level - 1;
                    break;
                }
            }
        }

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update matrices
        glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
        glm::mat4 model = glm::mat4(1.0f); // Identity matrix
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 1.0f)); // Desired transformation for model is to rotate 55 degrees around the axis
        rotation += rotationAmount * deltaTime;
        rotation = std::fmod(rotation, 360); // To prevent overflow

        // Send updated matrices to shader before draw
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

		// Load and bind vertex attributes and indices from meshes before draw
        for (unsigned int i = 0; i < modelCount; ++i)
        {
            models[i][modelLODs[i]].draw(shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (unsigned int i = 0; i < modelCount; ++i)
    {
        for (unsigned int j = 0; j < levelsOfDetail; ++j)
        {
            models[i][j].cleanUp();
		}
    }

	glDeleteProgram(shader);
    glfwTerminate();

    return 0;
}
