#ifndef MESH_H
#define MESH_H

#include <GL\GL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

using namespace std;

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;      
};

class Mesh {
public:
    vector <Vertex>       vertices;
    vector <unsigned int> indices;
    unsigned int VAO;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices)
    {}

    void Draw()
    {}

private:
    unsigned int VBO, EBO;

    void setupMesh()
    {}
};
#endif


