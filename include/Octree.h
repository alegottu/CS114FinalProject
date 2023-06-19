#ifndef OCTREE_H
#define OCTREE_H

#include <glm/glm.hpp>

#include <vector>
#include <cmath>

#include "AABB.h"
#include "Model.h"

const unsigned int maxDepth = levelsOfDetail;

struct Node
{
	const AABB boundingBox;

	Node(const AABB& boundingBox) : boundingBox(boundingBox) {}
};

struct Branch : Node
{
	Node** children;

	Branch(const AABB& boundingBox) 
		: Node(boundingBox)
	{
		children = new Node * [8];
	}
};

struct Leaf : Node
{
	std::vector<unsigned int> models; // Contains the indices for models within this node

	Leaf(const AABB& boundingBox, const std::vector<unsigned int>& models) : Node(boundingBox), models(models) {}
};

// Assumes origin is at zero
static Node* build(const AABB& bbox, const std::vector<unsigned int>& models, const glm::vec3* modelPositions, const unsigned int modelCount, const unsigned int depth)
{
	if (depth >= maxDepth)
	{
		return new Leaf(bbox, models);
	}

	Branch* current = new Branch(bbox);
	glm::vec3 center = bbox.center;
    glm::vec3 xMove = glm::vec3(1.0f, 0.0f, 0.0f) * bbox.extents.x;
    glm::vec3 yMove = glm::vec3(0.0f, 1.0f, 0.0f) * bbox.extents.y;
    glm::vec3 zMove = glm::vec3(0.0f, 0.0f, 1.0f) * bbox.extents.z;

	// Octants defined in clockwise order from the bottom-left
	AABB octants[8] = {
		AABB( bbox.min, center ),
		AABB( bbox.min + xMove, center + xMove ),
		AABB( center - yMove, bbox.max - yMove ),
		AABB( bbox.min + zMove, center + zMove ),
		AABB( bbox.min + yMove, center + yMove ),
		AABB( center - zMove, bbox.max - zMove ),
		AABB( center, bbox.max ),
		AABB( center - xMove, bbox.max - xMove )
	};

	std::vector<unsigned int> nextModels[8];
	
	for (unsigned int i = 0; i < modelCount; ++i)
	{
		for (unsigned int j = 0; j < 8; ++j)
		{
			if (octants[j].overlaps(modelPositions[i]))
			{
				nextModels[j].push_back(i);
			}
		}
	}

	for (unsigned int i = 0; i < 8; ++i)
	{
		current->children[i] = build(octants[i], nextModels[i], modelPositions, modelCount, depth + 1);
	}

	return current;
}

static void destructNode(Node* node, const unsigned int depth)
{
	if (depth == maxDepth - 1)
	{
		delete node;
	}
	else
	{
		Branch* current = (Branch*)node;

        for (int i = 0; i < 8; ++i)
        {
            destructNode(current->children[i], depth + 1);
        }

        delete node;
	}
}

static void setLevelsOfDetail(Branch* terminalBranch, unsigned int* modelLODs, const unsigned int levelOfDetail)
{
	for (unsigned int i = 0; i < 8; ++i)
	{
		Leaf* current = (Leaf*)terminalBranch->children[i];
		const std::vector<unsigned int> models = current->models;

		for (unsigned int j = 0; j < models.size(); ++j)
		{
			modelLODs[models[j]] = levelOfDetail;
		}
	}
}

static void findLevelsOfDetail(Node* const root, unsigned int* modelLODs, const glm::vec3& cameraPosition)
{
	Node* current = root;
	unsigned int worstDetail = maxDepth - 1;

	for (unsigned int level = worstDetail; level > 0; --level)
	{
		Branch* node = (Branch*)current;
		glm::vec3 center = node->boundingBox.center;
		
		// Calculate a number from 0 to 7 that will choose the correct octant where the camera resides
		short xDifference = !std::signbit(cameraPosition.x - center.x);
		short yDifference = !std::signbit(cameraPosition.y - center.y) * 4;
		short zDifference = !std::signbit(cameraPosition.z - center.z);
		zDifference = zDifference + 2 * (1 - xDifference);
		short nextChild = xDifference + yDifference + zDifference;

		current = node->children[nextChild];

		// For the remaining octants that aren't chosen, their models will be left with a worse level of detail 
		for (unsigned int i = 0; i < 8; ++i)
		{
			if (i == nextChild)
			{
				continue;
			}

			Branch* next = node;

			for (unsigned int j = level; j > 0; --j)
			{
				next = (Branch*)next->children[i];
			}

			setLevelsOfDetail(next, modelLODs, level);
		}
	}

	setLevelsOfDetail((Branch*)current, modelLODs, 0);
}

#endif
