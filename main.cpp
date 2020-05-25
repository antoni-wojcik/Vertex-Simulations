//
//  main.cpp
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 14/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#define RETINA
#define SCR_WIDTH 1200
#define SCR_HEIGHT 800
#define ITERATION_LENGTH_MAX 3.0f
#define ITERATION_LENGTH_MIN 0.001f
#define ITERATION_LENGTH_STRENGTH 3.0f

#define MIN_FRAME_TIME 0.003f
#define MAX_FRAME_COUNT 6


#include <iostream>
#include <fstream>

// include the OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "cloth.h"
#include "camera.h"


// function declarations
GLFWwindow* initialiseOpenGL();
void framebufferSizeCallback(GLFWwindow*, int, int);
void mouseCallback(GLFWwindow*, double, double);
void mouseButtonCallback(GLFWwindow*, int, int, int);
void scrollCallback(GLFWwindow*, double, double);
void processInput(GLFWwindow*);
void takeScreenshot(const std::string& name = "screenshot", bool show_image = false);
void countFPS(float);

#ifdef RETINA
// dimensions of the viewport (they have to be multiplied by 2 at the retina displays)
unsigned int scr_width = SCR_WIDTH*2;
unsigned int scr_height = SCR_HEIGHT*2;
#else
unsigned int scr_width = SCR_WIDTH;
unsigned int scr_height = SCR_HEIGHT;
#endif

// variables used in the main loop
float last_frame_time = 0.0f;
float delta_time = 0.0f;
float lag = 0.0f;
bool stopping = false;
bool run = true;

// fps counter variables
float fps_sum = 0.0f;
const int fps_steps = 5;
int fps_steps_counter = 0;

// variables used in callbacks
bool mouse_first_check = true;
bool mouse_hidden = true;
float mouse_last_x = scr_width / 2.0f;
float mouse_last_y = scr_width / 2.0f;

// screenshot variables
bool taking_screenshot = false;

// camera pointer
Camera* camera;

int main(int argc, const char * argv[]) {
    GLFWwindow* window = initialiseOpenGL();
    
    camera = new Camera(60.0f, (float)scr_width / (float)scr_height, glm::vec3(-3.5f), 45.0f, 45.0f);
    
    KernelGL* cloth = new Cloth(500, 500, 0.01f, 1.0f, 500.0f, 0.2f, glm::vec3(0.0f), 0.03f, "src/shaders/cloth.vs", "src/shaders/cloth.gs", "src/shaders/cloth.fs", "src/kernels/kernel_cloth.ocl");
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glEnable(GL_DEPTH_TEST);
    
    while(!glfwWindowShouldClose(window)) {
        float current_time = glfwGetTime();
        delta_time = current_time - last_frame_time;
        last_frame_time = current_time;
        lag += delta_time;
        
        glClearColor(0.7f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        processInput(window);
        
        cloth->draw(camera);
        
        if(run) {
            if(lag >= MIN_FRAME_TIME) {
                int steps = (int)(lag / MIN_FRAME_TIME);
                lag -= steps * MIN_FRAME_TIME;
                if(steps > MAX_FRAME_COUNT) steps = MAX_FRAME_COUNT;
                cloth->iterate(steps);
            }
        } else {
            lag = 0.0f;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        glFinish();
    }
    
    delete cloth;
    delete camera;
    
    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    scr_width = width;
    scr_height = height;
    
    camera->setSize((float)width / (float)height);
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera->move(FORWARD, delta_time);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera->move(BACK, delta_time);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera->move(LEFT, delta_time);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera->move(RIGHT, delta_time);
    
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera->setFasterSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera->setFasterSpeed(false);
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera->setSlowerSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera->setSlowerSpeed(false);
    
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if(!taking_screenshot) takeScreenshot();
        taking_screenshot = true;
    } else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
        taking_screenshot = false;
    }
    
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if(!stopping) run = !run;
        stopping = true;
    } else if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        stopping = false;
    }
}

void mouseCallback(GLFWwindow* window, double pos_x, double pos_y) {
    if(mouse_first_check) {
        mouse_last_x = pos_x;
        mouse_last_y = pos_y;
        mouse_first_check = false;
    }
    
    float offset_x = pos_x - mouse_last_x;
    float offset_y = mouse_last_y - pos_y;
    
    mouse_last_x = pos_x;
    mouse_last_y = pos_y;
    
    if(mouse_hidden) camera->rotate(offset_x, -offset_y);
}

void scrollCallback(GLFWwindow* window, double offset_x, double offset_y) {
    camera->zoom(offset_y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if(mouse_hidden) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        mouse_hidden = !mouse_hidden;
    }
}

void countFPS(float delta_time) {
    
    // count fps
    if(fps_steps_counter == fps_steps) {
        fps_steps_counter = 0;
        fps_sum = 0;
    }
    fps_sum += delta_time;
    fps_steps_counter++;
}

GLFWwindow* initialiseOpenGL() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    
    GLFWwindow* window;
    
    #ifdef RETINA
    window = glfwCreateWindow(scr_width/2, scr_height/2, "Clouds", NULL, NULL);
    #else
    window = glfwCreateWindow(scr_width, scr_height, "Clouds", NULL, NULL);
    #endif
    if(window == NULL) {
        std::cout << "ERROR: OpenGL: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    
    glfwMakeContextCurrent(window);
    
    // set the callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    
    // tell GLFW to capture the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if(glewInit() != GLEW_OK) {
        std::cerr << "ERROR: OpenGL: Failed to initialize GLEW" << std::endl;
        glGetError();
        exit(-1);
    }
    
    return window;
}

void takeScreenshot(const std::string& name, bool show_image) {
    std::cout << "Taking screenshot: " << name << ".tga " << ", dimensions: " << scr_width << ", " << scr_height << std::endl;
    short TGA_header[] = {0, 2, 0, 0, 0, 0, (short)scr_width, (short)scr_height, 24};
    char* pixel_data = new char[3 * scr_width * scr_height]; //there are 3 colors (RGB) for each pixel
    std::ofstream file("screenshots/" + name + ".tga", std::ios::out | std::ios::binary);
    if(!pixel_data || !file) {
        std::cerr << "ERROR: COULD NOT TAKE THE SCREENSHOT" << std::endl;
        exit(-1);
    }
    
    glFinish();
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, scr_width, scr_height, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
    glFinish();
    
    file.write((char*)TGA_header, 9 * sizeof(short));
    file.write(pixel_data, 3 * scr_width * scr_height);
    file.close();
    delete [] pixel_data;
    
    if(show_image) {
        std::cout << "Opening the screenshot" << std::endl;
        std::system(("open " + name + ".tga").c_str());
    }
}
