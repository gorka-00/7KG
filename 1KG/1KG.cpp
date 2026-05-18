#define GLEW_DLL
#define GLFW_DLL

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode, fragmentCode;
        std::ifstream vFile(vertexPath), fFile(fragmentPath);
        std::stringstream vStream, fStream;

        vStream << vFile.rdbuf();
        fStream << fFile.rdbuf();

        vertexCode = vStream.str();
        fragmentCode = fStream.str();

        const char* vCode = vertexCode.c_str();
        const char* fCode = fragmentCode.c_str();

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vCode, NULL);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fCode, NULL);
        glCompileShader(fs);

        ID = glCreateProgram();
        glAttachShader(ID, vs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void use() { glUseProgram(ID); }

    void setMat4(const char* name, const glm::mat4& mat) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setMat3(const char* name, const glm::mat3& mat) {
        glUniformMatrix3fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setVec3(const char* name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(value));
    }

    void setVec3(const char* name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(ID, name), x, y, z);
    }

    void setFloat(const char* name, float value) {
        glUniform1f(glGetUniformLocation(ID, name), value);
    }
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO, VBO, EBO;

    Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds) {
        vertices = verts;
        indices = inds;
        setupMesh();
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glBindVertexArray(0);
    }
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::vector<glm::mat4> meshTransforms;

    Model(const std::string& path) {
        loadModel(path);
        meshTransforms.resize(meshes.size(), glm::mat4(1.0f));
    }

    void Draw(Shader& shader) {
        for (size_t i = 0; i < meshes.size(); ++i) {
            shader.setMat4("model", meshTransforms[i]);

            glm::mat3 normalMatrix =
                glm::transpose(glm::inverse(glm::mat3(meshTransforms[i])));

            shader.setMat3("normalMatrix", normalMatrix);

            meshes[i].Draw();
        }
    }

private:
    void loadModel(const std::string& path) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        return Mesh(vertices, indices);
    }
};

struct ObjectTransform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 pivot = glm::vec3(0.0f);
};

std::vector<ObjectTransform> transforms(4);

glm::vec3 cameraPos(3.0f, 6.0f, 12.0f);
glm::vec3 cameraFront(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = -20.0f;

bool firstMouse = true;

float lastX = 640.0f;
float lastY = 360.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

glm::mat4 calculateModelMatrix(int index) {
    glm::mat4 model = glm::mat4(1.0f);

    if (index == 0)
        return model;

    // 1 деталь
    if (index == 1) {
        model = glm::translate(model, transforms[1].position);

        model = glm::translate(model, transforms[1].pivot);
        model = glm::rotate(model, glm::radians(transforms[1].rotation.z), glm::vec3(0, 0, 1));
        model = glm::translate(model, -transforms[1].pivot);

        return model;
    }

    // 2 деталь
    if (index == 2) {
        model = calculateModelMatrix(1);

        model = glm::translate(
            model,
            glm::vec3(
                0.0f,
                transforms[2].position.y,
                0.0f
            )
        );

        return model;
    }

    // 3 деталь
    if (index == 3) {
        model = calculateModelMatrix(2);

        model = glm::translate(model, transforms[3].pivot);
        model = glm::rotate(model, glm::radians(transforms[3].rotation.x), glm::vec3(1, 0, 0));
        model = glm::translate(model, -transforms[3].pivot);

        return model;
    }

    return model;
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lab 7", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    Model ourModel("laba3.obj");
    Shader shader("vertex.glsl", "fragment.glsl");

    // pivot 1 детали
    transforms[1].pivot = glm::vec3(
        0.16218f,
        -0.26813f,
        1.8763f
    );

    // pivot 2 детали
    transforms[2].pivot = glm::vec3(
        0.77075f,
        0.994173f,
        0.65241f
    );

    // pivot 3 детали
    transforms[3].pivot = glm::vec3(
        0.77075f,
        0.994173f,
        0.65241f
    );

    shader.use();

    shader.setVec3("light.position", 2.0f, 3.0f, 4.0f);
    shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    shader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    shader.setVec3("material.ambient", 0.0215f, 0.1745f, 0.0215f);
    shader.setVec3("material.diffuse", 0.07568f, 0.61424f, 0.07568f);
    shader.setVec3("material.specular", 0.633f, 0.727811f, 0.633f);

    shader.setFloat("material.shininess", 76.8f);

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float speed = 3.0f * deltaTime;
        float rotSpeed = 90.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // камера
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += speed * cameraFront;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= speed * cameraFront;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

        // движение 1 детали
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            transforms[1].position.z += speed;

        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
            transforms[1].position.z -= speed;

        // ограничения 1 детали
        if (transforms[1].position.z < -3.045168f)
            transforms[1].position.z = -3.045168f;

        if (transforms[1].position.z > 0.99764f)
            transforms[1].position.z = 0.99764f;

        // движение 2 детали вверх вниз
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            transforms[2].position.y += speed;

        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            transforms[2].position.y -= speed;

        // ограничения 2 детали
        if (transforms[2].position.y > 0.0f)
            transforms[2].position.y = 0.0f;

        if (transforms[2].position.y < -0.6f)
            transforms[2].position.y = -0.6f;

        // вращение 3 детали
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            transforms[3].rotation.x += rotSpeed;

        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            transforms[3].rotation.x -= rotSpeed;

        // ограничения 3 детали
        if (transforms[3].rotation.x > 90.0f)
            transforms[3].rotation.x = 90.0f;

        if (transforms[3].rotation.x < -90.0f)
            transforms[3].rotation.x = -90.0f;

        glClearColor(0.12f, 0.12f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            1280.0f / 720.0f,
            0.1f,
            100.0f
        );

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", cameraPos);

        for (size_t i = 0; i < ourModel.meshes.size(); ++i)
            ourModel.meshTransforms[i] = calculateModelMatrix((int)i);

        ourModel.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}