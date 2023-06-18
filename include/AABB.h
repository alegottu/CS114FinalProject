#ifndef AABB_H
#define AABB_H

#include <glm/vec3.hpp>

struct AABB
{
	AABB(const glm::vec3& min, const glm::vec3& max);
	AABB(const glm::vec3& center, const glm::vec3& extents, bool fromExtents);

	const bool overlaps(const AABB& other) const;
	const bool overlaps(const glm::vec3& point) const;

	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;
	glm::vec3 extents;
};

#endif
