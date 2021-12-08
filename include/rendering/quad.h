#ifndef QUAD
#define QUAD

#include <glm/glm.hpp>
#include <rendering/mesh.h>

#include <vector>

// positions
std::vector<glm::vec3> quadPositions {
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

/*
class Quad : public Mesh {

public:
    Quad() : Mesh(quadPositions, quadUVs, quadIndices){}
};

*/
#endif // QUAD