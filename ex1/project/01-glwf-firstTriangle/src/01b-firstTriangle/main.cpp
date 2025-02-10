#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <vector>

// Structure to hold vertex data (position and color)
struct Vertex {
    GLfloat x, y;   // Position
    GLfloat r, g, b; // Color
};

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ---------------------------------------------------------------------------
// Low-Poly Panda Vertices & Indices
// ---------------------------------------------------------------------------
//
// The panda is broken down into several parts. Each part is either a polygon
// or simple triangles. All vertices are placed in one big vector. Then we
// define a matching list of indices to tell OpenGL how to form the triangles.
//
// Layout (by index ranges in vertices):
//   Head main shape:     indices [0..5]    (white)
//   Left ear:            indices [6..8]    (black)
//   Right ear:           indices [9..11]   (black)
//   Left eye patch:      indices [12..14]  (black)
//   Right eye patch:     indices [15..17]  (black)
//   Nose:                indices [18..20]  (black)
//   Body main shape:     indices [21..26]  (white)
//   Left arm:            indices [27..30]  (black)
//   Right arm:           indices [31..34]  (black)
//   Left leg:            indices [35..38]  (black)
//   Right leg:           indices [39..42]  (black)
// ---------------------------------------------------------------------------
std::vector<Vertex> vertices = {
    // -------------------- HEAD (white) --------------------
    // A simple "fan"-able polygon with 6 points:
    // 0
    {  0.0f,   0.80f,  1.0f, 1.0f, 1.0f },
    // 1
    { -0.25f,  0.65f,  1.0f, 1.0f, 1.0f },
    // 2
    { -0.30f,  0.50f,  1.0f, 1.0f, 1.0f },
    // 3
    {  0.00f,  0.45f,  1.0f, 1.0f, 1.0f },
    // 4
    {  0.30f,  0.50f,  1.0f, 1.0f, 1.0f },
    // 5
    {  0.25f,  0.65f,  1.0f, 1.0f, 1.0f },

    // -------------------- LEFT EAR (black) ----------------
    // 6
    { -0.25f,  0.80f,  0.0f, 0.0f, 0.0f },
    // 7
    { -0.40f,  0.85f,  0.0f, 0.0f, 0.0f },
    // 8
    { -0.25f,  0.65f,  0.0f, 0.0f, 0.0f },

    // -------------------- RIGHT EAR (black) ---------------
    // 9
    {  0.25f,  0.80f,  0.0f, 0.0f, 0.0f },
    // 10
    {  0.40f,  0.85f,  0.0f, 0.0f, 0.0f },
    // 11
    {  0.25f,  0.65f,  0.0f, 0.0f, 0.0f },

    // -------------------- LEFT EYE PATCH (black) ----------
    // 12
    { -0.15f,  0.60f,  0.0f, 0.0f, 0.0f },
    // 13
    { -0.22f,  0.55f,  0.0f, 0.0f, 0.0f },
    // 14
    { -0.08f,  0.55f,  0.0f, 0.0f, 0.0f },

    // -------------------- RIGHT EYE PATCH (black) ---------
    // 15
    {  0.15f,  0.60f,  0.0f, 0.0f, 0.0f },
    // 16
    {  0.22f,  0.55f,  0.0f, 0.0f, 0.0f },
    // 17
    {  0.08f,  0.55f,  0.0f, 0.0f, 0.0f },

    // -------------------- NOSE (black) ---------------------
    // 18
    {  0.00f,  0.52f,  0.0f, 0.0f, 0.0f },
    // 19
    { -0.03f,  0.50f,  0.0f, 0.0f, 0.0f },
    // 20
    {  0.03f,  0.50f,  0.0f, 0.0f, 0.0f },

    // -------------------- BODY (white) ---------------------
    // A 6-point polygon for the main body
    // 21
    { -0.20f,  0.45f,  1.0f, 1.0f, 1.0f },
    // 22
    {  0.20f,  0.45f,  1.0f, 1.0f, 1.0f },
    // 23
    {  0.35f,  0.00f,  1.0f, 1.0f, 1.0f },
    // 24
    {  0.20f, -0.30f,  1.0f, 1.0f, 1.0f },
    // 25
    { -0.20f, -0.30f,  1.0f, 1.0f, 1.0f },
    // 26
    { -0.35f,  0.00f,  1.0f, 1.0f, 1.0f },

    // -------------------- LEFT ARM (black) -----------------
    // 27
    { -0.20f,  0.45f,  0.0f, 0.0f, 0.0f },
    // 28
    { -0.35f,  0.40f,  0.0f, 0.0f, 0.0f },
    // 29
    { -0.40f,  0.15f,  0.0f, 0.0f, 0.0f },
    // 30
    { -0.25f,  0.20f,  0.0f, 0.0f, 0.0f },

    // -------------------- RIGHT ARM (black) ----------------
    // 31
    {  0.20f,  0.45f,  0.0f, 0.0f, 0.0f },
    // 32
    {  0.35f,  0.40f,  0.0f, 0.0f, 0.0f },
    // 33
    {  0.40f,  0.15f,  0.0f, 0.0f, 0.0f },
    // 34
    {  0.25f,  0.20f,  0.0f, 0.0f, 0.0f },

    // -------------------- LEFT LEG (black) -----------------
    // A simple rectangle as two triangles
    // 35
    { -0.20f, -0.30f,  0.0f, 0.0f, 0.0f },
    // 36
    { -0.20f, -0.55f,  0.0f, 0.0f, 0.0f },
    // 37
    { -0.10f, -0.55f,  0.0f, 0.0f, 0.0f },
    // 38
    { -0.10f, -0.30f,  0.0f, 0.0f, 0.0f },

    // -------------------- RIGHT LEG (black) ----------------
    // 39
    {  0.20f, -0.30f,  0.0f, 0.0f, 0.0f },
    // 40
    {  0.20f, -0.55f,  0.0f, 0.0f, 0.0f },
    // 41
    {  0.10f, -0.55f,  0.0f, 0.0f, 0.0f },
    // 42
    {  0.10f, -0.30f,  0.0f, 0.0f, 0.0f },
};

// Define how to connect the vertices above into triangles:
std::vector<unsigned int> indices = {
    // Head main shape (fan out from vertex 0)
    // (0,1,2), (0,2,3), (0,3,4), (0,4,5)
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5,

    // Left ear
    6, 7, 8,

    // Right ear
    9, 10, 11,

    // Left eye patch
    12, 13, 14,

    // Right eye patch
    15, 16, 17,

    // Nose
    18, 19, 20,

    // Body (fan out from vertex 21)
    // (21,22,23), (21,23,24), (21,24,25), (21,25,26)
    21, 22, 23,
    21, 23, 24,
    21, 24, 25,
    21, 25, 26,

    // Left arm (2 triangles)
    27, 28, 29,
    27, 29, 30,

    // Right arm (2 triangles)
    31, 32, 33,
    31, 33, 34,

    // Left leg (2 triangles)
    35, 36, 37,
    35, 37, 38,

    // Right leg (2 triangles)
    39, 40, 41,
    39, 41, 42
};

// ---------------------------------------------------------------------------
// OpenGL objects
// ---------------------------------------------------------------------------
unsigned int VBO, VAO, EBO;
unsigned int shaderProgram;

// Vertex shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;
void main()
{
    FragColor = vec4(ourColor, 1.0f);
}
)";

// Function to initialize shaders
void initShaders() {
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create and link shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Function to initialize buffers (VBO, VAO, EBO)
void initBuffers() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Generate and bind VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
        vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Generate and bind EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    // Unbind (good practice)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Function to render the scene
void renderScene() {
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Use OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Low-Poly Panda", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Build and compile our shader program
    initShaders();
    // Initialize VBO, VAO, EBO
    initBuffers();

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Input
        processInput(window);

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw our panda
        renderScene();

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}

// process all input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// glfw: callback when window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
