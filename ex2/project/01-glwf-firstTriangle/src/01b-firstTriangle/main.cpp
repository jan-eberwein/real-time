/***********************************************************************************
 * AnimatedPanda.cpp
 *
 * Combines the original (non-animated) geometry with animation transforms:
 *   - Head tilts when you move the mouse left/right.
 *   - Arms and legs swing periodically.
 *   - The whole panda "bobs" up and down slightly.
 *
 * The geometry is divided into separate parts (head, ears, eyes, nose, body, arms, legs),
 * exactly matching the coordinates from the original non-animated version so that it
 * looks identical - only now it moves!
 ***********************************************************************************/

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>                 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>        
#include <iostream>
#include <vector>
#include <cmath>

 // Window dimensions
static const unsigned int SCR_WIDTH = 800;
static const unsigned int SCR_HEIGHT = 600;

// Track mouse position for head tilt
static double mouseX = SCR_WIDTH / 2.0;
static double mouseY = SCR_HEIGHT / 2.0;

// Simple struct for holding 2D position + RGB color
struct Vertex {
    GLfloat x, y;    // Position
    GLfloat r, g, b; // Color
};

// --------------------------------------------------------------------------------
// Shader sources
// --------------------------------------------------------------------------------
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

// Uniforms for transformations
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0f);
}
)";

// --------------------------------------------------------------------------------
// Utility functions
// --------------------------------------------------------------------------------
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void processInput(GLFWwindow* window);

// Compile+link the shaders into a program
unsigned int createShaderProgram(const char* vsrc, const char* fsrc)
{
    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsrc, NULL);
    glCompileShader(vertexShader);

    // Check for vertex compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader failed:\n" << infoLog << std::endl;
    }

    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fsrc, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader failed:\n" << infoLog << std::endl;
    }

    // Link into a program
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glLinkProgram(prog);

    // Check for linking errors
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed:\n" << infoLog << std::endl;
    }

    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return prog;
}

// Create VAO, VBO, EBO for a given set of vertices/indices
void setupVAO(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO,
    const std::vector<Vertex>& verts,
    const std::vector<unsigned int>& inds)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// --------------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------------
int main()
{
    // GLFW init
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // For OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Animated Low-Poly Panda", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // GLAD init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Build and use shader program
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    // Enable alpha blending (though we draw opaque here, it's often useful)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---------------------------------------------------------------------------
    // Define the Panda geometry EXACTLY as in the original code (non-animated).
    // Each part is taken from the same coordinates, ensuring the same look.
    // We'll create separate VAOs so we can transform each part independently.
    // ---------------------------------------------------------------------------

    // Head (indices 0..5) - white
    std::vector<Vertex> headVertices = {
        {  0.0f,  0.80f, 1.0f, 1.0f, 1.0f },
        { -0.25f, 0.65f, 1.0f, 1.0f, 1.0f },
        { -0.30f, 0.50f, 1.0f, 1.0f, 1.0f },
        {  0.00f, 0.45f, 1.0f, 1.0f, 1.0f },
        {  0.30f, 0.50f, 1.0f, 1.0f, 1.0f },
        {  0.25f, 0.65f, 1.0f, 1.0f, 1.0f },
    };
    std::vector<unsigned int> headIndices = {
        0,1,2,
        0,2,3,
        0,3,4,
        0,4,5
    };

    // Left ear (indices 6..8) - black
    std::vector<Vertex> leftEarVertices = {
        { -0.25f, 0.80f, 0.0f, 0.0f, 0.0f },
        { -0.40f, 0.85f, 0.0f, 0.0f, 0.0f },
        { -0.25f, 0.65f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> leftEarIndices = {
        0,1,2
    };

    // Right ear (indices 9..11) - black
    std::vector<Vertex> rightEarVertices = {
        { 0.25f, 0.80f, 0.0f, 0.0f, 0.0f },
        { 0.40f, 0.85f, 0.0f, 0.0f, 0.0f },
        { 0.25f, 0.65f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> rightEarIndices = {
        0,1,2
    };

    // Left eye patch (indices 12..14) - black
    std::vector<Vertex> leftEyeVertices = {
        { -0.15f, 0.60f, 0.0f, 0.0f, 0.0f },
        { -0.22f, 0.55f, 0.0f, 0.0f, 0.0f },
        { -0.08f, 0.55f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> leftEyeIndices = {
        0,1,2
    };

    // Right eye patch (indices 15..17) - black
    std::vector<Vertex> rightEyeVertices = {
        { 0.15f,  0.60f, 0.0f, 0.0f, 0.0f },
        { 0.22f,  0.55f, 0.0f, 0.0f, 0.0f },
        { 0.08f,  0.55f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> rightEyeIndices = {
        0,1,2
    };

    // Nose (indices 18..20) - black
    std::vector<Vertex> noseVertices = {
        {  0.0f,  0.52f, 0.0f, 0.0f, 0.0f },
        { -0.03f, 0.50f, 0.0f, 0.0f, 0.0f },
        {  0.03f, 0.50f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> noseIndices = {
        0,1,2
    };

    // Body (indices 21..26) - white
    std::vector<Vertex> bodyVertices = {
        { -0.20f,  0.45f, 1.0f, 1.0f, 1.0f },
        {  0.20f,  0.45f, 1.0f, 1.0f, 1.0f },
        {  0.35f,  0.00f, 1.0f, 1.0f, 1.0f },
        {  0.20f, -0.30f, 1.0f, 1.0f, 1.0f },
        { -0.20f, -0.30f, 1.0f, 1.0f, 1.0f },
        { -0.35f,  0.00f, 1.0f, 1.0f, 1.0f },
    };
    std::vector<unsigned int> bodyIndices = {
        0,1,2,
        0,2,3,
        0,3,4,
        0,4,5
    };

    // Left arm (indices 27..30) - black
    // 4 vertices => 2 triangles
    std::vector<Vertex> leftArmVertices = {
        { -0.20f, 0.45f, 0.0f, 0.0f, 0.0f },
        { -0.35f, 0.40f, 0.0f, 0.0f, 0.0f },
        { -0.40f, 0.15f, 0.0f, 0.0f, 0.0f },
        { -0.25f, 0.20f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> leftArmIndices = {
        0,1,2,
        0,2,3
    };

    // Right arm (indices 31..34) - black
    std::vector<Vertex> rightArmVertices = {
        {  0.20f, 0.45f, 0.0f, 0.0f, 0.0f },
        {  0.35f, 0.40f, 0.0f, 0.0f, 0.0f },
        {  0.40f, 0.15f, 0.0f, 0.0f, 0.0f },
        {  0.25f, 0.20f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> rightArmIndices = {
        0,1,2,
        0,2,3
    };

    // Left leg (indices 35..38) - black
    // A rectangle, made of 4 vertices => 2 triangles
    std::vector<Vertex> leftLegVertices = {
        { -0.20f, -0.30f, 0.0f, 0.0f, 0.0f },
        { -0.20f, -0.55f, 0.0f, 0.0f, 0.0f },
        { -0.10f, -0.55f, 0.0f, 0.0f, 0.0f },
        { -0.10f, -0.30f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> leftLegIndices = {
        0,1,2,
        0,2,3
    };

    // Right leg (indices 39..42) - black
    std::vector<Vertex> rightLegVertices = {
        {  0.20f, -0.30f, 0.0f, 0.0f, 0.0f },
        {  0.20f, -0.55f, 0.0f, 0.0f, 0.0f },
        {  0.10f, -0.55f, 0.0f, 0.0f, 0.0f },
        {  0.10f, -0.30f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<unsigned int> rightLegIndices = {
        0,1,2,
        0,2,3
    };

    // --------------------------------------------------------------------------------
    // Setup VAOs for each part
    // --------------------------------------------------------------------------------
    unsigned int headVAO, headVBO, headEBO;
    setupVAO(headVAO, headVBO, headEBO, headVertices, headIndices);

    unsigned int leftEarVAO, leftEarVBO, leftEarEBO;
    setupVAO(leftEarVAO, leftEarVBO, leftEarEBO, leftEarVertices, leftEarIndices);

    unsigned int rightEarVAO, rightEarVBO, rightEarEBO;
    setupVAO(rightEarVAO, rightEarVBO, rightEarEBO, rightEarVertices, rightEarIndices);

    unsigned int leftEyeVAO, leftEyeVBO, leftEyeEBO;
    setupVAO(leftEyeVAO, leftEyeVBO, leftEyeEBO, leftEyeVertices, leftEyeIndices);

    unsigned int rightEyeVAO, rightEyeVBO, rightEyeEBO;
    setupVAO(rightEyeVAO, rightEyeVBO, rightEyeEBO, rightEyeVertices, rightEyeIndices);

    unsigned int noseVAO, noseVBO, noseEBO;
    setupVAO(noseVAO, noseVBO, noseEBO, noseVertices, noseIndices);

    unsigned int bodyVAO, bodyVBO, bodyEBO;
    setupVAO(bodyVAO, bodyVBO, bodyEBO, bodyVertices, bodyIndices);

    unsigned int leftArmVAO, leftArmVBO, leftArmEBO;
    setupVAO(leftArmVAO, leftArmVBO, leftArmEBO, leftArmVertices, leftArmIndices);

    unsigned int rightArmVAO, rightArmVBO, rightArmEBO;
    setupVAO(rightArmVAO, rightArmVBO, rightArmEBO, rightArmVertices, rightArmIndices);

    unsigned int leftLegVAO, leftLegVBO, leftLegEBO;
    setupVAO(leftLegVAO, leftLegVBO, leftLegEBO, leftLegVertices, leftLegIndices);

    unsigned int rightLegVAO, rightLegVBO, rightLegEBO;
    setupVAO(rightLegVAO, rightLegVBO, rightLegEBO, rightLegVertices, rightLegIndices);

    // --------------------------------------------------------------------------------
    // Set up 2D orthographic projection & simple view
    // --------------------------------------------------------------------------------
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f,  // left, right
        -1.0f, 1.0f,  // bottom, top
        -1.0f, 1.0f); // near, far
    glm::mat4 view = glm::mat4(1.0f);

    // Retrieve uniform locations for once
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Feed the projection/view transforms (they're constant)
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // --------------------------------------------------------------------------------
    // Animation loop
    // --------------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Timing
        float currentTime = glfwGetTime();

        // Input
        processInput(window);

        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // A slight "bobbing" for the entire panda
        float bobbing = 0.03f * std::sin(currentTime * 2.0f);

        // Build a global transform that we'll apply to all parts (for bobbing)
        glm::mat4 globalModel(1.0f);
        globalModel = glm::translate(globalModel, glm::vec3(0.0f, bobbing, 0.0f));

        // For the head tilt, rotate based on mouse X in the range [-20deg, 20deg].
        float normX = static_cast<float>(mouseX) / SCR_WIDTH * 2.0f - 1.0f; // -1..1
        float maxRotation = glm::radians(20.0f);
        float headRotation = normX * maxRotation;

        // Arms and legs swinging
        float swingSpeed = 3.0f;
        float armSwing = 0.3f * std::sin(currentTime * swingSpeed);
        // For legs, shift the phase by pi so they move opposite arms:
        float legSwing = 0.3f * std::sin(currentTime * swingSpeed + glm::pi<float>());

        // ------------------------------------------------------------------------
        // BODY: the reference for everything else.
        // The body doesn't rotate around a pivot—just the global bob.
        // ------------------------------------------------------------------------
        glm::mat4 bodyModel = globalModel;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
        glBindVertexArray(bodyVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)bodyIndices.size(), GL_UNSIGNED_INT, 0);

        // ------------------------------------------------------------------------
        // HEAD & features: pivot at the bottom-center of the head (y=0.45).
        // We'll rotate about (0,0.45).
        // ------------------------------------------------------------------------
        glm::mat4 headModel = globalModel;
        // Translate pivot to origin
        headModel = glm::translate(headModel, glm::vec3(0.0f, 0.45f, 0.0f));
        // Rotate
        headModel = glm::rotate(headModel, headRotation, glm::vec3(0.0f, 0.0f, 1.0f));
        // Translate back
        headModel = glm::translate(headModel, glm::vec3(0.0f, -0.45f, 0.0f));

        // HEAD
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(headModel));
        glBindVertexArray(headVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)headIndices.size(), GL_UNSIGNED_INT, 0);

        // LEFT EAR (same transform as head)
        glBindVertexArray(leftEarVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)leftEarIndices.size(), GL_UNSIGNED_INT, 0);

        // RIGHT EAR (same transform as head)
        glBindVertexArray(rightEarVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)rightEarIndices.size(), GL_UNSIGNED_INT, 0);

        // LEFT EYE PATCH
        glBindVertexArray(leftEyeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)leftEyeIndices.size(), GL_UNSIGNED_INT, 0);

        // RIGHT EYE PATCH
        glBindVertexArray(rightEyeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)rightEyeIndices.size(), GL_UNSIGNED_INT, 0);

        // NOSE
        glBindVertexArray(noseVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)noseIndices.size(), GL_UNSIGNED_INT, 0);

        // ------------------------------------------------------------------------
        // LEFT ARM: pivot at top of the arm ( -0.20, 0.45 )
        // ------------------------------------------------------------------------
        glm::mat4 leftArmModel = globalModel;
        // Move pivot to origin
        leftArmModel = glm::translate(leftArmModel, glm::vec3(-0.20f, 0.45f, 0.0f));
        // Rotate
        leftArmModel = glm::rotate(leftArmModel, armSwing, glm::vec3(0.0f, 0.0f, 1.0f));
        // Move back
        leftArmModel = glm::translate(leftArmModel, glm::vec3(0.20f, -0.45f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftArmModel));
        glBindVertexArray(leftArmVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)leftArmIndices.size(), GL_UNSIGNED_INT, 0);

        // ------------------------------------------------------------------------
        // RIGHT ARM: pivot at top of the arm ( 0.20, 0.45 )
        // ------------------------------------------------------------------------
        glm::mat4 rightArmModel = globalModel;
        rightArmModel = glm::translate(rightArmModel, glm::vec3(0.20f, 0.45f, 0.0f));
        rightArmModel = glm::rotate(rightArmModel, -armSwing, glm::vec3(0.0f, 0.0f, 1.0f));
        rightArmModel = glm::translate(rightArmModel, glm::vec3(-0.20f, -0.45f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightArmModel));
        glBindVertexArray(rightArmVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)rightArmIndices.size(), GL_UNSIGNED_INT, 0);

        // ------------------------------------------------------------------------
        // LEFT LEG: pivot at top of the leg ( -0.20, -0.30 )
        // ------------------------------------------------------------------------
        glm::mat4 leftLegModel = globalModel;
        leftLegModel = glm::translate(leftLegModel, glm::vec3(-0.20f, -0.30f, 0.0f));
        leftLegModel = glm::rotate(leftLegModel, legSwing, glm::vec3(0.0f, 0.0f, 1.0f));
        leftLegModel = glm::translate(leftLegModel, glm::vec3(0.20f, 0.30f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftLegModel));
        glBindVertexArray(leftLegVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)leftLegIndices.size(), GL_UNSIGNED_INT, 0);

        // ------------------------------------------------------------------------
        // RIGHT LEG: pivot at top of the leg ( 0.20, -0.30 )
        // ------------------------------------------------------------------------
        glm::mat4 rightLegModel = globalModel;
        rightLegModel = glm::translate(rightLegModel, glm::vec3(0.20f, -0.30f, 0.0f));
        rightLegModel = glm::rotate(rightLegModel, -legSwing, glm::vec3(0.0f, 0.0f, 1.0f));
        rightLegModel = glm::translate(rightLegModel, glm::vec3(-0.20f, 0.30f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightLegModel));
        glBindVertexArray(rightLegVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)rightLegIndices.size(), GL_UNSIGNED_INT, 0);

        // Unbind
        glBindVertexArray(0);

        // Swap buffers and poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &headVAO);
    glDeleteBuffers(1, &headVBO);
    glDeleteBuffers(1, &headEBO);

    glDeleteVertexArrays(1, &leftEarVAO);
    glDeleteBuffers(1, &leftEarVBO);
    glDeleteBuffers(1, &leftEarEBO);

    glDeleteVertexArrays(1, &rightEarVAO);
    glDeleteBuffers(1, &rightEarVBO);
    glDeleteBuffers(1, &rightEarEBO);

    glDeleteVertexArrays(1, &leftEyeVAO);
    glDeleteBuffers(1, &leftEyeVBO);
    glDeleteBuffers(1, &leftEyeEBO);

    glDeleteVertexArrays(1, &rightEyeVAO);
    glDeleteBuffers(1, &rightEyeVBO);
    glDeleteBuffers(1, &rightEyeEBO);

    glDeleteVertexArrays(1, &noseVAO);
    glDeleteBuffers(1, &noseVBO);
    glDeleteBuffers(1, &noseEBO);

    glDeleteVertexArrays(1, &bodyVAO);
    glDeleteBuffers(1, &bodyVBO);
    glDeleteBuffers(1, &bodyEBO);

    glDeleteVertexArrays(1, &leftArmVAO);
    glDeleteBuffers(1, &leftArmVBO);
    glDeleteBuffers(1, &leftArmEBO);

    glDeleteVertexArrays(1, &rightArmVAO);
    glDeleteBuffers(1, &rightArmVBO);
    glDeleteBuffers(1, &rightArmEBO);

    glDeleteVertexArrays(1, &leftLegVAO);
    glDeleteBuffers(1, &leftLegVBO);
    glDeleteBuffers(1, &leftLegEBO);

    glDeleteVertexArrays(1, &rightLegVAO);
    glDeleteBuffers(1, &rightLegVBO);
    glDeleteBuffers(1, &rightLegEBO);

    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// --------------------------------------------------------------------------------
// Callbacks
// --------------------------------------------------------------------------------
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
