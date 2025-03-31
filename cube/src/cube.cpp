#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>

// Incluir GLM para las operaciones matemáticas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback para ajustar el viewport al cambiar el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Función para compilar y enlazar un shader simple
unsigned int createShaderProgram() {
    // Código fuente del vertex shader con transformación MVP
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 uMVP; // Matriz Model-View-Projection
        void main()
        {
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";

    // Código fuente del fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0); // Color anaranjado
        }
    )";

    int success;
    char infoLog[512];

    // Compilar vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Error compilando vertex shader: " << infoLog << std::endl;
    }

    // Compilar fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Error compilando fragment shader: " << infoLog << std::endl;
    }

    // Crear y enlazar el shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Error enlazando shader program: " << infoLog << std::endl;
    }

    // Eliminar shaders compilados
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Error al inicializar GLFW" << std::endl;
        return -1;
    }

    // Configurar versión de OpenGL (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear la ventana
    GLFWwindow* window = glfwCreateWindow(800, 600, "Cubo OBJ", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Inicializar GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // Habilitar el test de profundidad
    glEnable(GL_DEPTH_TEST);

    // Cargar el modelo OBJ utilizando Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("cube.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error cargando el modelo: " << importer.GetErrorString() << std::endl;
        return -1;
    }

    // Asumimos que el modelo tiene una sola malla
    aiMesh* mesh = scene->mMeshes[0];

    // Extraer los vértices y los índices
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Crear buffers y configurar atributos de vértices
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    // Buffer de vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    // Buffer de índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // Configurar atributo: posición (3 floats por vértice)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Crear el shader program
    unsigned int shaderProgram = createShaderProgram();

    // Bucle principal de renderizado
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Limpiar buffers de color y profundidad
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calcular las matrices de transformación con GLM
        // Model: Rotar el cubo en función del tiempo
        glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                      (float)glfwGetTime(),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
        // View: trasladar la escena para "alejar" la cámara
        glm::mat4 view = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(0.0f, 0.0f, -12.0f));
        // Projection: matriz de proyección en perspectiva
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                800.0f / 600.0f,
                                                0.1f,
                                                100.0f);
        // Matriz MVP: Projection * View * Model
        glm::mat4 mvp = projection * view * model;

        // Usar el shader y enviar la matriz MVP al uniform
        glUseProgram(shaderProgram);
        int mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        // Dibujar el cubo
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Liberar recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

