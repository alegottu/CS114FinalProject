#include "Model.h"

Mesh::Mesh()
	: vertices(nullptr), numVertices(0), indices(nullptr), numIndices(0), textures(nullptr), numTextures(0)
{
}

Mesh::Mesh(const unsigned int numVertices, const unsigned int numIndices, const unsigned int numTextures)
	: numVertices(numVertices), numIndices(numIndices), numTextures(numTextures)
{
	vertices = new Vertex[numVertices];
	indices = new unsigned int[numIndices];
	textures = new unsigned int[numTextures];
}

Mesh::~Mesh()
{
	delete[] vertices;
	delete[] indices;
	delete[] textures;
}

Mesh Mesh::operator=(const Mesh& mesh)
{
	vertices = mesh.vertices;
	indices = mesh.indices;
	textures = mesh.textures;

	return mesh;
}

Model::Model(Mesh* meshes, const unsigned int numMeshes)
	: meshes(meshes), numMeshes(numMeshes)
{
}
