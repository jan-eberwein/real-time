#pragma once
#ifndef WINDOW_H
#define WINDOW_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h> // holds all OpenGL type declarations
#include <GLFW/glfw3.h>

//#include <nanogui/nanogui.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream> 
#include <iomanip>
#include <optional>


GLFWwindow* window = nullptr;
bool gui = false;


// utility function to derminate the window
// ---------------------------------------------------
void DestroyWindow(void)
{
    //if (screen) delete screen;
    if (window)
    {
        glfwTerminate(); //delete window;
    }
}


// utility function to instantiate a GLFW3 window
// ---------------------------------------------------
int InitWindow(const int width, const int height, const char *appname = "OpenGL" )
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    // glsl_version = "#version 440";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(width, height, appname, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);


    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    return 0;
}


// utility function to instantiate a GLFW3 window and a GUI 
// ---------------------------------------------------
int InitWindowAndGUI(const int &width, const int &height, const char* appname = "OpenGL")
{
    if (InitWindow(width, height, appname) >= 0)
    {

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char *glsl_version = "#version 430";
        ImGui_ImplOpenGL3_Init(glsl_version);

        gui = true;
    }
    else
        return -1;
}

bool firstUpdate = true;
void UpdateWindow(float deltaTime = 0.0f)
{
    
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
}


GLFWframebuffersizefun global_fbsize_fun;
void SetFramebufferSizeCallback(GLFWframebuffersizefun fun)
{
    global_fbsize_fun = fun;
    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow* window_, int width, int height) {
            global_fbsize_fun(window_, width, height);
        }
    );
}

GLFWcursorposfun global_cursorpos_fun;
void SetCursorPosCallback(GLFWcursorposfun fun)
{
    global_cursorpos_fun = fun;
    glfwSetCursorPosCallback(window,
        [](GLFWwindow* window_, double x, double y) {
            if (!gui || !ImGui::GetIO().WantCaptureMouse) 
                global_cursorpos_fun(window_, x, y);
        }
    );
}

GLFWscrollfun global_scroll_fun;
void SetScrollCallback(GLFWscrollfun fun)
{
    global_scroll_fun = fun;
    glfwSetScrollCallback(window,
        [](GLFWwindow* window_, double x, double y) {
            if (!gui || !ImGui::GetIO().WantCaptureMouse)
                global_scroll_fun(window_, x, y);
        }
    );
}

GLFWmousebuttonfun global_mousbutton_fun;
void SetMouseButtonCallback(GLFWmousebuttonfun fun)
{
    global_mousbutton_fun = fun;
    glfwSetMouseButtonCallback(window,
        [](GLFWwindow* w_, int button, int action, int modifiers) {
            if (!gui || !ImGui::GetIO().WantCaptureMouse)
                global_mousbutton_fun(w_, button, action, modifiers);
        }
    );
}

GLFWkeyfun global_key_fun;
void SetKeyCallback(GLFWkeyfun fun)
{
    global_key_fun = fun;
    glfwSetKeyCallback(window,
        [](GLFWwindow* w, int key, int scancode, int action, int mods) {
            if (!gui || !ImGui::GetIO().WantCaptureKeyboard)
                global_key_fun(w, key, scancode, action, mods);
        }
    );
}

GLFWcharfun global_char_fun;
void SetCharCallback(GLFWcharfun fun)
{
    global_char_fun = fun;
    glfwSetCharCallback(window,
        [](GLFWwindow*w, unsigned int codepoint) {
            if (!gui || !ImGui::GetIO().WantCaptureKeyboard)
                global_char_fun(w, codepoint);
        }
    );
}

GLFWdropfun global_drop_fun;
void SetDropCallback(GLFWdropfun fun)
{
    global_drop_fun = fun;
    glfwSetDropCallback(window,
        [](GLFWwindow* w, int count, const char** filenames) {
            global_drop_fun(w, count, filenames);
        }
    );
}




#endif
