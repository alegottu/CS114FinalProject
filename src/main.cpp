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
#include <sstream>
#include <cmath>

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

static void loadTextures(const char** filePaths, const unsigned int amount, const unsigned int shader)
{
    const unsigned int _amount = 2;
    unsigned int textures[_amount];
    glGenTextures(amount, textures);

    for (unsigned int i = 0; i < amount; ++i)
    {
        // Bind and set texture properties
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load texture image
        int width, height, channels;
        unsigned char* data = stbi_load(filePaths[i], &width, &height, &channels, 0);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
        }

        // Set uniform sampler variable for shader and free image data
        glUniform1i(glGetUniformLocation(shader, ("tex" + std::to_string(i)).c_str()), i); // Possibly change order of this
        stbi_image_free(data);
    }
}

static Mesh processMesh(const aiMesh* mesh)
{
    unsigned int numVertices = mesh->mNumVertices;
    Mesh result(numVertices, mesh->mNumFaces * 3, 1);
    // * 3 assumes all primitives are triangles

    for (unsigned int i = 0; i < numVertices; ++i)
    {
        aiVector3D vertex = mesh->mVertices[i];
        Vertex newVertex;
        newVertex.position = glm::vec3(vertex.x, vertex.y, vertex.z);
        aiVector3D normal = mesh->mNormals[i];
        newVertex.normal = glm::vec3(normal.x, normal.y, normal.z);
        result.vertices[i] = newVertex;
    }

    if (mesh->mTextureCoords[0] != nullptr)
    {
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            aiVector3D texCoords = mesh->mTextureCoords[0][i];
            result.vertices[i].texCoords = glm::vec2(texCoords.x, texCoords.y);
        }
    }

    return result;
}

static void processNode(const aiNode* node, const aiScene* scene, Mesh* meshes, unsigned int currentMesh)
{
    // Process meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes[currentMesh++] = processMesh(mesh);
    }

    // Recursively process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene, meshes, currentMesh);
    }
}

static Model loadModel(const char* path)
{
	Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (scene == nullptr || scene->mFlags != NULL && AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
    {
        std::cout << "Error loading model: " << importer.GetErrorString() << std::endl;
        return Model(nullptr, -1);
    }

    const unsigned int numMeshes = scene->mNumMeshes;
    Mesh* result = new Mesh[numMeshes];
    processNode(scene->mRootNode, scene, result, 0);
    return Model(result, numMeshes);
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
    glfwSetCursorPos(window, SCR_WIDTH/2, SCR_HEIGHT/2);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Successfully loaded OpenGL version " << glGetString(GL_VERSION)  << " function pointers" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);

    // Load models
    Model bunny = loadModel("res/bunny.obj");
    
    // Set up camera and matrices
    float rotationAmount = 45.0f; float rotation = 0.0f; // For rotating model over time
    float fov = 45.0f; float near = 0.1f; float far = 100.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, near, far);

    // Load shaders
    const std::string vertexSource = parseShader("res/shaders/shader.vs");
    const std::string fragmentSource = parseShader("res/shaders/shader.fs");
    unsigned int shader = createShader(vertexSource, fragmentSource);
    glUseProgram(shader);

    // Find uniform locations to send matrices to shaders later
    int modelLocation = glGetUniformLocation(shader, "model");
    int viewLocation = glGetUniformLocation(shader, "view");
    int projectionLocation = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    // Generate buffers
    unsigned int array, vertexBuffer;
    glGenVertexArrays(1, &array);
	glBindVertexArray(array);
    glGenBuffers(1, &vertexBuffer);
    unsigned int elementBuffer;
    glGenBuffers(1, &elementBuffer);

    // Enable vertex attributes for shaders
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)( 3 * sizeof(float) )); // Offset for start of texture coordinates
    glEnableVertexAttribArray(1);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) 
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        handleInput(window);

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update matrices
        glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
        glm::mat4 model = glm::mat4(1.0f); // Identity matrix
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(1.0f)); // Desired transformation for model is to rotate 55 degrees around the axis
        rotation += rotationAmount * deltaTime;
        rotation = std::fmod(rotation, 360); // To prevent overflow

        // Send updated matrices to shader before draw
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

		// Load and bind vertex attributes and indices from meshes
        for (unsigned int i = 0; i < bunny.numMeshes; ++i)
        {
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            Mesh mesh = bunny.meshes[i];
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.numVertices, mesh.vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.numIndices * sizeof(unsigned int), mesh.indices, GL_STATIC_DRAW);

			glDrawElements(GL_TRIANGLES, mesh.numIndices / 3, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &array);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &elementBuffer);
    glDeleteProgram(shader);
    glfwTerminate();

    return 0;
}
