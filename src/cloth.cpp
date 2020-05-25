//
//  cloth.cpp
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 25/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include <GL/glew.h>
#include "cloth.h"

#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include <vector>
#include <string>


Cloth::ClothProperties::ClothProperties(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt) {
    size_x = x;
    size_y = y;
    length = l;
    mass = m;
    stiffness = k;
    damping = b;
    pos = p;
    time_step = dt;
}

void Cloth::ClothProperties::updateModelMatrix(const Camera* camera) {
    model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, pos - camera->getPosition());
    model_matrix = glm::scale(model_matrix, glm::vec3(1.0f, -1.0f, 1.0f));
}

Cloth::Cloth(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt, const char* cloth_vs_path, const char* cloth_gs_path, const char* cloth_fs_path, const char* kernel_path, const char* kernel_pos_name, const char* kernel_vel_name) : cloth_prop(x, y, l, m, k, b, p, dt), shader(cloth_vs_path, cloth_fs_path, cloth_gs_path) {
    try {
        initialiseOpenCL();
        buildProgram(kernel_path);
        createKernels(kernel_pos_name, kernel_vel_name);
        createGLBuffers();
        createCLBuffers();
    } catch(cl::Error e) {
        processError(e);
    }
}

Cloth::~Cloth() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Cloth::createGLBuffers() {
    int size_x = cloth_prop.size_x;
    int size_y = cloth_prop.size_y;
    indices_num = (size_x - 1) * (size_y - 1) * 6;
    buff_size = cloth_prop.size_x * cloth_prop.size_y * 3 * sizeof(cl_float);
    
    float* vertices = new float[size_x * size_y * 3];
    unsigned int* indices = new unsigned int[indices_num];
    
    float half_width = (float)((size_x - 1) * cloth_prop.length) * 0.5f;
    float half_height = (float)((size_y - 1) * cloth_prop.length) * 0.5f;
    
    // create vertices of the cloth
    
    for(int j = 0; j < size_y; j++) for(int i = 0; i < size_x; i++) {
        vertices[(j * size_x + i) * 3]     = -half_width  + (float)i * cloth_prop.length;
        vertices[(j * size_x + i) * 3 + 1] =  0.0f;
        vertices[(j * size_x + i) * 3 + 2] =  half_height - (float)j * cloth_prop.length;
    }
    
    // create indices to set drawing order of the triangle vertices
    
    for(int j = 0; j < size_y - 1; j++) for(int i = 0; i < size_x - 1; i++) {
        // first triangle
        indices[(j * (size_x - 1) + i) * 6]     = j * size_x + i;
        indices[(j * (size_x - 1) + i) * 6 + 1] = j * size_x + i + 1;
        indices[(j * (size_x - 1) + i) * 6 + 2] = (j + 1) * size_x + i;
        
        // second triangle
        indices[(j * (size_x - 1) + i) * 6 + 3] = j * size_x + i + 1;
        indices[(j * (size_x - 1) + i) * 6 + 4] = (j + 1) * size_x + i + 1;
        indices[(j * (size_x - 1) + i) * 6 + 5] = (j + 1) * size_x + i;
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buff_size, vertices, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_num * sizeof(float), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    delete [] vertices;
    delete [] indices;
}

void Cloth::createCLBuffers() {
    buff_pos_prev = cl::Buffer(context, CL_MEM_READ_WRITE, buff_size);
    buff_pos_next = cl::BufferGL(context, CL_MEM_READ_WRITE, VBO);
    buff_vel_prev = cl::Buffer(context, CL_MEM_READ_WRITE, buff_size);
    buff_vel_next = cl::Buffer(context, CL_MEM_READ_WRITE, buff_size);
    
    int size = cloth_prop.size_x * cloth_prop.size_x * 3;
    float* vel = new float[size];
    for(int i = 0; i < size; i++) vel[i] = 0.0f;
    
    cl::CommandQueue queue(context, device);
    queue.enqueueWriteBuffer(buff_vel_prev, CL_TRUE, 0, buff_size, vel);
    queue.enqueueCopyBuffer(buff_pos_next, buff_pos_prev, 0, 0, buff_size);
    queue.enqueueBarrierWithWaitList();
    
    setConstKernelArgs();
    
    // set the velocity step from 0 to 1/2
    
    queue.enqueueNDRangeKernel(kernel_vel, cl::NDRange(1, 1), cl::NDRange(size_t(cloth_prop.size_x - 2), size_t(cloth_prop.size_y - 2)), cl::NullRange);
    queue.enqueueCopyBuffer(buff_vel_next, buff_vel_prev, 0, 0, buff_size);
    queue.enqueueBarrierWithWaitList();
    
    kernel_vel.setArg(8, cloth_prop.time_step);
    
    queue.finish();
    
    delete [] vel;
}

void Cloth::createKernels(const char* kernel_pos_name, const char* kernel_vel_name) {
    // create the kernels given the names
    
    kernel_pos = cl::Kernel(program, kernel_pos_name);
    kernel_vel = cl::Kernel(program, kernel_vel_name);
}

void Cloth::setConstKernelArgs() {
    kernel_pos.setArg(0, buff_pos_prev);
    kernel_pos.setArg(1, buff_pos_next);
    kernel_pos.setArg(2, buff_vel_prev);
    kernel_pos.setArg(3, cloth_prop.size_x);
    kernel_pos.setArg(4, cloth_prop.size_y);
    kernel_pos.setArg(5, cloth_prop.time_step);
    
    kernel_vel.setArg(0, buff_vel_prev);
    kernel_vel.setArg(1, buff_vel_next);
    kernel_vel.setArg(2, buff_pos_prev);
    kernel_vel.setArg(3, cloth_prop.size_x);
    kernel_vel.setArg(4, cloth_prop.size_y);
    kernel_vel.setArg(5, cloth_prop.length);
    
    float effective_stiffness = cloth_prop.stiffness / cloth_prop.mass;
    float effective_damping = cloth_prop.damping / cloth_prop.mass;
    
    kernel_vel.setArg(6, effective_stiffness);
    kernel_vel.setArg(7, effective_damping);
    kernel_vel.setArg(8, cloth_prop.time_step * 0.5f);
}

void Cloth::iterate(int steps = 1) {
    try {
        std::vector<cl::Memory> mem_objs;
        mem_objs.push_back(buff_pos_next);
        
        // use the leapfrog algorithm
        
        cl::CommandQueue queue(context, device);
        
        for(int i = 0; i < steps; i++) {
        
            // calculate new position
        
            // make sure the OpenGL has released the buffer
            queue.enqueueAcquireGLObjects(&mem_objs);
            queue.enqueueNDRangeKernel(kernel_pos, cl::NDRange(1, 1), cl::NDRange(size_t(cloth_prop.size_x - 2), size_t(cloth_prop.size_y - 2)), cl::NullRange);
            queue.enqueueCopyBuffer(buff_pos_next, buff_pos_prev, 0, 0, buff_size);
            queue.enqueueReleaseGLObjects(&mem_objs);
            queue.enqueueBarrierWithWaitList();
            
            // calculate new velocity
            
            queue.enqueueNDRangeKernel(kernel_vel, cl::NDRange(1, 1), cl::NDRange(size_t(cloth_prop.size_x - 2), size_t(cloth_prop.size_y - 2)), cl::NullRange);
            queue.enqueueCopyBuffer(buff_vel_next, buff_vel_prev, 0, 0, buff_size);
            queue.enqueueBarrierWithWaitList();
        }
        
        queue.finish();
    } catch(cl::Error e) {
        processError(e);
    }
}

void Cloth::draw(const Camera* camera) {
    shader.use();
    
    cloth_prop.updateModelMatrix(camera);
    
    shader.setMat4("PVM", camera->getPVMatrix() * cloth_prop.model_matrix);
    shader.setMat3("M_normals", glm::mat3(glm::transpose(glm::inverse(cloth_prop.model_matrix))));
    shader.setVec3("camera_dir", camera->getNormal());
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices_num, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
