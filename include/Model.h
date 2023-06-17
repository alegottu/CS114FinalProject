#ifndef MODEL_H
#define MODEL_H 

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>

const unsigned int levelsOfDetail = 2;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Texture
{
	bool isSpecular;
	unsigned int id;
	const char* file;
	const char* uniform;

	Texture();
	Texture(unsigned int id, bool isSpecular, const char* file, const char* uniform);
};

class Mesh
{
	public:
		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
		
		void draw(const unsigned int shader) const;
		void cleanUp();
	
	private:

		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		unsigned int vertexArray, vertexBuffer, elementBuffer = 0; // IDs for each
};

class Model
{
	public:
		Model(std::vector<Mesh> meshes);

		void draw(const unsigned int shader) const;
		void cleanUp(); // Not in destructor because cannot be called more than once as a result of local variables going out of scope; handles OpenGL clean up

	private:
		std::vector<Mesh> meshes;
};

#endif
