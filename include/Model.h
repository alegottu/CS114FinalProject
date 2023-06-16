#ifndef MODEL_H
#define MODEL_H 

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

// Maybe change all ptrs to const
struct Mesh
{
	Vertex* vertices;
	const unsigned int numVertices;
	unsigned int* indices;
	const unsigned int numIndices;
	unsigned int* textures; // IDs for all textures
	const unsigned int numTextures;

	Mesh();
	Mesh(const unsigned int numVertices, const unsigned int numIndices, const unsigned int numTextures);
	~Mesh();
	Mesh operator=(const Mesh& mesh);
};

struct Model
{
	Mesh* meshes;
	const unsigned int numMeshes;

	Model(Mesh* mesh, const unsigned int numMeshes);
};

#endif
