#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/vec3.hpp>

// Consider switching to data oriented structure of arrays to represent all transforms
struct Camera
{
    public:
    Camera(glm::vec3 position)
		: position(position)
    {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
    }

    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;

    const float speed = 5.0f;
    float yaw; // Angle to determine forward direction
    float pitch; // Angle to determine up direction
};

#endif