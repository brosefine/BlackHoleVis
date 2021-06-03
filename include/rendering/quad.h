#pragma once

#include <glm/glm.hpp>

#include <vector>

#include <rendering/mesh.h>

// positions
std::vector<glm::vec3> quadPostions {
    {-1.0f, -1.0f, 0.0f},
    {-1.0f, 1.0f, 0.0f},
    {1.0f, -1.0f, 0.0f},
    {1.0f, 1.0f, 0.0f}
};

// texCoords
std::vector<glm::vec2> quadUVs {
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f}
};

std::vector<unsigned int> quadIndices{
    0, 1, 2,
    2, 1, 3
};