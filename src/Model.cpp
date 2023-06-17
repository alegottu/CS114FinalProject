#include <glad/glad.h>

#include <iostream>

#include "Model.h"

Texture::Texture()
    : isSpecular(false), id(0), file(nullptr), uniform(nullptr)
{
}

Texture::Texture(unsigned int id, bool isSpecular, const char* file, const char* uniform)
	: isSpecular(isSpecular), id(id), file(file), uniform(uniform)
{
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : vertices(vertices), indices(indices), textures(textures)
{
	glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &elementBuffer);

    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // vertex positions
    size_t size = sizeof(Vertex);
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, size, (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, size, (void*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);	
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)offsetof(Vertex, texCoords));

    glBindVertexArray(0);
}


void Mesh::draw(const unsigned int shader) const
{
	for (unsigned int i = 0; i < textures.size(); ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
		glUniform1i(glGetUniformLocation(shader, textures[i].uniform), i); // Set uniform sampler in shader
	}

	glBindVertexArray(vertexArray);
	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::cleanUp()
{
	glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &elementBuffer);
}

Model::Model(std::vector<Mesh> meshes)
    : meshes(meshes)
{
}

void Model::draw(const unsigned int shader) const
{
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		meshes[i].draw(shader);
	}
}

void Model::cleanUp()
{
    for (unsigned int i = 0; i < meshes.size(); ++i)
    {
        meshes[i].cleanUp();
    }
}
