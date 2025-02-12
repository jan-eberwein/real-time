#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/assets.h>
#include <util/window.h>

#include <iostream>
#include <vector>

// Callback declarations
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void renderQuad();
void renderCube();

// settings
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const char *APP_NAME = "deferred";

int main()
{
    // Settings for deferred shading
    bool animateLights = true;
    bool displayGBuffers = false;
    int gBufferToDisplay = 0;
    int numLights = 32;
    float lightboxAlpha = 0.5f;
    float gamma = 1.6f;

    // New UI parameters for vignette post–processing
    bool vignetteOn = true;
    float vignetteStrength = 0.5f; // 0 = no effect, 1 = full effect

    // glfw: initialize and configure
    // ------------------------------
    InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME, false); // avoid resizing the window!
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    SetCursorPosCallback(mouse_callback);
    SetMouseButtonCallback(mouse_button_callback);
    SetScrollCallback(scroll_callback);
    SetFramebufferSizeCallback([](GLFWwindow *window, int w, int h)
    {
        if (w > 0 && h > 0)
        {
            SCR_WIDTH = w;
            SCR_HEIGHT = h;
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        }
    });

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    const std::string SRC = "../src/09b-deferred-solution/";

    Shader shaderGeometryPass(SRC + "g_buffer.vs", SRC + "g_buffer.fs");
    Shader shaderLightingPass(SRC + "deferred_shading.vs", SRC + "deferred_shading.fs");
    Shader shaderLightBox(SRC + "deferred_light_box.vs", SRC + "deferred_light_box.fs");
    Shader shaderDebug(SRC + "fbo_debug.vs", SRC + "fbo_debug.fs");
    // New shader for vignette post–processing:
    Shader shaderVignette(SRC + "vignette.vs", SRC + "vignette.fs");

    // load models
    // -----------
    stbi_set_flip_vertically_on_load(true);
    Model myModel("../resources/objects/backpack/backpack.obj", true);

    // positions for several objects in the scene
    std::vector<glm::vec3> objectPositions;
    objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));

    // configure g-buffer framebuffer
    // ------------------------------
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "GBuffer Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Configure post–processing framebuffer (for vignette effect)
    // -----------------------------------------------------------
    unsigned int ppFBO;
    glGenFramebuffers(1, &ppFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ppFBO);
    unsigned int ppColorBuffer;
    glGenTextures(1, &ppColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ppColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppColorBuffer, 0);
    // also create a renderbuffer object for depth and stencil attachment
    unsigned int rboPP;
    glGenRenderbuffers(1, &rboPP);
    glBindRenderbuffer(GL_RENDERBUFFER, rboPP);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboPP);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Postprocessing framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting info
    // -------------
    const unsigned int NR_LIGHTS = 128;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    std::vector<glm::vec4> lightDirs;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        float xPos = ((rand() % 100) / 100.0f) * 8.0f - 4.0f;
        float yPos = ((rand() % 100) / 100.0f) * 6.0f - 4.0f;
        float zPos = ((rand() % 100) / 100.0f) * 8.0f - 4.0f;
        lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        float rColor = ((rand() % 100) / 200.0f) + 0.5f;
        float gColor = ((rand() % 100) / 200.0f) + 0.5f;
        float bColor = ((rand() % 100) / 200.0f) + 0.5f;
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
        float xDir = ((rand() % 100) / 100.0f) * 2.0f - 1.0f;
        float yDir = ((rand() % 100) / 100.0f) * 2.0f - 1.0f;
        float zDir = ((rand() % 100) / 100.0f) * 2.0f - 1.0f;
        float aniOffset = ((rand() % 100) / 100.0f) * glm::pi<float>();
        lightDirs.push_back(glm::vec4(xDir, yDir, zDir, aniOffset));
    }

    // shader configuration
    // --------------------
    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Poll events and process input
        glfwPollEvents();
        processInput(window);

        if (gui)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin(APP_NAME);
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::SliderFloat("gamma", &gamma, 0.1f, 5.0f);
            ImGui::Checkbox("animate lights", &animateLights);
            ImGui::SliderInt("number of lights", &numLights, 1, 128);
            ImGui::SliderFloat("lightbox alpha", &lightboxAlpha, 0.0f, 1.0f);
            ImGui::Checkbox("display GBuffers", &displayGBuffers);
            if (displayGBuffers)
                ImGui::SliderInt("show GBuffer", &gBufferToDisplay, 0, 2);

            // New UI for vignette effect
            ImGui::Checkbox("Vignette", &vignetteOn);
            ImGui::SliderFloat("Vignette Strength", &vignetteStrength, 0.0f, 1.0f);

            if (ImGui::Button("reload shaders"))
            {
                shaderGeometryPass.reload();
                shaderLightingPass.reload();
                shaderLightBox.reload();
                shaderDebug.reload();
                shaderVignette.reload();
                shaderLightingPass.use();
                shaderLightingPass.setInt("gPosition", 0);
                shaderLightingPass.setInt("gNormal", 1);
                shaderLightingPass.setInt("gAlbedoSpec", 2);
            }
            ImGui::End();
            ImGui::Render();
        }

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Geometry pass: render scene's geometry/color data into gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        for (unsigned int i = 0; i < objectPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, objectPositions[i]);
            model = glm::scale(model, glm::vec3(0.5f));
            shaderGeometryPass.setMat4("model", model);
            myModel.Draw(shaderGeometryPass);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (displayGBuffers)
        {
            // For debugging: display one of the GBuffer attachments
            shaderDebug.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
            shaderDebug.setInt("fboAttachment", gBufferToDisplay);
            renderQuad();
        }
        else
        {
            // 2. Lighting and Light Boxes Pass: render to post–processing framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, ppFBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Lighting pass using the G-buffer textures
            shaderLightingPass.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
            for (unsigned int i = 0; i < lightPositions.size(); i++)
            {
                glm::vec3 pos = lightPositions[i];
                if (animateLights)
                    pos += glm::vec3(lightDirs[i]) * std::sinf(currentFrame + lightDirs[i].w);
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", pos);
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            }
            shaderLightingPass.setVec3("viewPos", camera.Position);
            shaderLightingPass.setInt("numLights", numLights);
            shaderLightingPass.setFloat("gamma", gamma);
            renderQuad();

            // Render light boxes on top of the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_CULL_FACE);
            shaderLightBox.use();
            shaderLightBox.setMat4("projection", projection);
            shaderLightBox.setMat4("view", view);
            shaderLightBox.setFloat("alpha", lightboxAlpha);
            for (unsigned int i = 0; i < numLights; i++)
            {
                glm::vec3 pos = lightPositions[i];
                if (animateLights)
                    pos += glm::vec3(lightDirs[i]) * std::sinf(currentFrame + lightDirs[i].w);
                model = glm::mat4(1.0f);
                model = glm::translate(model, pos);
                model = glm::scale(model, glm::vec3(0.125f));
                shaderLightBox.setMat4("model", model);
                shaderLightBox.setVec3("lightColor", lightColors[i]);
                renderCube();
            }
            glDisable(GL_BLEND);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 3. Post–processing: Apply vignette effect
            glClear(GL_COLOR_BUFFER_BIT);
            shaderVignette.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ppColorBuffer);
            shaderVignette.setBool("vignetteOn", vignetteOn);
            shaderVignette.setFloat("vignetteStrength", vignetteStrength);
            renderQuad();
        }

        // Render the GUI on top
        if (gui)
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// renderCube() renders a 1x1 3D cube in NDC.
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
             1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
            -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f, 0.0f,  0.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  1.0f, -1.0f, 0.0f,  0.0f,  0.0f, 0.0f,
            -1.0f,  1.0f,  1.0f, -1.0f, 0.0f,  0.0f,  1.0f, 0.0f,
            // right face
             1.0f,  1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0.0f,
             1.0f, -1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
             1.0f,  1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  0.0f, 0.0f,
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f,  1.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f,
            -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f,  0.0f, 0.0f
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    if (leftButtonDown)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            leftButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            middleButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            rightButtonDown = true;
    }
    else if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            leftButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            middleButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            rightButtonDown = false;
    }
}
