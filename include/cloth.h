//
//  cloth.h
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 25/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef cloth_h
#define cloth_h

#include "kernelgl.h"
#include "shader.h"

#include "glm.hpp"

class Cloth : public KernelGL {
private:
    struct ClothProperties {
        int size_x, size_y;
        float length; // distance between vertices
        float mass, stiffness, damping;
        float time_step;
        glm::vec3 pos;
        glm::mat4 model_matrix;
        
        ClothProperties(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt);
        void updateModelMatrix(const Camera* camera);
        
    } cloth_prop;
    
    // OpenGL related variables
    
    GLuint VBO, VAO, EBO;
    GLsizei indices_num;
    
    Shader shader;
    
    // OpenCL related variables
    
    cl::Kernel kernel_pos;
    cl::Kernel kernel_vel;
    
    cl::Buffer buff_pos_prev;
    cl::BufferGL buff_pos_next;
    cl::Buffer buff_vel_prev;
    cl::Buffer buff_vel_next;
    
    size_t buff_size;
    
    void createGLBuffers();
    void createCLBuffers();
    void createKernels(const char* kernel_pos_name, const char* kernel_vel_name);
    void setConstKernelArgs();
    
public:
    Cloth(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt, const char* cloth_vs_path, const char* cloth_gs_path, const char* cloth_fs_path, const char* kernel_path, const char* kernel_pos_name, const char* kernel_vel_name);
    
    ~Cloth();
    
    virtual void iterate(int steps);
    
    virtual void draw(const Camera* camera);
};

#endif /* cloth_h */
