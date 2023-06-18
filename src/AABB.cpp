#include <glm/glm.hpp>

#include "AABB.h"

// assumes that min is always lower and father away than max
AABB::AABB(const glm::vec3& min, const glm::vec3& max)
	: min(min), max(max)
{
	extents = (max - min) / 2.0f;
	center = min + extents;
}

AABB::AABB(const glm::vec3& center, const glm::vec3& extents, bool fromExtents)
	: center(center), extents(extents)
{
	min = center - extents;
	max = center + extents;
}

const bool AABB::overlaps(const AABB& other) const
{
	return
		min.x <= other.max.x &&
		max.x >= other.min.x &&
		min.y <= other.max.y &&
		max.y >= other.min.y &&
		min.z <= other.max.z &&
		max.z >= other.min.z;
}

const bool AABB::overlaps(const glm::vec3& point) const
{
	return
		point.x >= min.x &&
		point.x <= max.x &&
		point.y >= min.y &&
		point.y <= max.y &&
		point.z >= min.z &&
		point.z <= max.z;
}
