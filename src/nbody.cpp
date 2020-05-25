//
//  nbody.cpp
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 25/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include <GL/glew.h>
#include "nbody.h"

#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include <vector>
#include <string>

#define KERNEL_POS "iteratePos"
#define KERNEL_VEL "iterateVel"
#define KERNEL_DENS "calculateDens"
#define KERNEL_POT "calculatePot"
#define KERNEL_ACC "calculateAcc"
#define KERNEL_FFT_H "calculateFFTH"

NBody::NBody(int g, int n, float m, const char* vs_path, const char* fs_path, const char* kernel_path) : KernelGL(kernel_path), grid_num(g), body_num(n), body_mass(m), shader(vs_path, fs_path) {
    try {
        createKernels();
        createGLBuffers();
        createCLBuffers();
    } catch(cl::Error e) {
        processError(e);
    }
}

NBody::~NBody() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void NBody::createGLBuffers() {
    buff_v_size = body_num * 3 * sizeof(cl_float);
    
    float* vertices = new float[body_num * 3];
    
    // create vertices of the cloth
    
    for(int i = 0; i < body_num * 3; i++) {
        vertices[i * 3]     = 0.0f; // TODO
        vertices[i * 3 + 1] = 0.0f; // TODO
        vertices[i * 3 + 2] = 0.0f; // TODO
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buff_v_size, vertices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    delete [] vertices;
}

void NBody::createCLBuffers() {
    buff_pos_0 = cl::Buffer(context, CL_MEM_READ_WRITE, buff_v_size);
    buff_pos_1 = cl::BufferGL(context, CL_MEM_READ_WRITE, VBO);
    buff_vel_0 = cl::Buffer(context, CL_MEM_READ_WRITE, buff_v_size);
    buff_vel_1 = cl::Buffer(context, CL_MEM_READ_WRITE, buff_v_size);
    buff_acc = cl::Buffer(context, CL_MEM_READ_WRITE, buff_v_size);
    
    buff_s_size = grid_num * grid_num * grid_num * sizeof(cl_float);
    
    buff_dens = cl::Buffer(context, CL_MEM_READ_WRITE, buff_s_size);
    buff_pot = cl::Buffer(context, CL_MEM_READ_WRITE, buff_s_size);
    buff_FFT_h = cl::Buffer(context, CL_MEM_READ_WRITE, buff_s_size);
    
    setConstKernelArgs();
    
    cl::Kernel kernel_FFT_h(program, KERNEL_FFT_H);
    
    // TODO set the args of the kernel_FFT_h
    kernel_FFT_h.setArg(0, buff_FFT_h);
    
    cl::CommandQueue queue(context, device);
    
    // copy buff_pos_1 to buff_pos_0 and fill buff_vel_0 with zeros
    
    queue.enqueueFillBuffer(buff_vel_0, 0, 0, buff_v_size);
    queue.enqueueCopyBuffer(buff_pos_1, buff_pos_0, 0, 0, buff_v_size);
    queue.enqueueBarrierWithWaitList();
    
    // calculate the fft_h to be later used to speed up claculations by convolution thm
    
    queue.enqueueNDRangeKernel(kernel_FFT_h, cl::NullRange, cl::NDRange(size_t(grid_num), size_t(grid_num), size_t(grid_num)), cl::NullRange);
    
    // increment velocity half the step
    
    queue.enqueueNDRangeKernel(kernel_vel, cl::NullRange, cl::NDRange(size_t(body_num), size_t(body_num), size_t(body_num)), cl::NullRange);
    queue.enqueueCopyBuffer(buff_vel_1, buff_vel_0, 0, 0, buff_v_size);
    
    queue.finish();
    
    // set time step to the full range
}

void NBody::createKernels() {
    kernel_pos = cl::Kernel(program, KERNEL_POS);
    kernel_vel = cl::Kernel(program, KERNEL_VEL);
    kernel_dens = cl::Kernel(program, KERNEL_DENS);
    kernel_pot = cl::Kernel(program, KERNEL_POT);
    kernel_acc = cl::Kernel(program, KERNEL_ACC);
}

void NBody::setConstKernelArgs() {
    //kernel_pos.setArg(0, buff_pos_prev);
    // TODO
}

void NBody::iterate(int steps) {
    try {
        std::vector<cl::Memory> mem_objs;
        mem_objs.push_back(buff_pos_1);
        
        // TODO
        
    } catch(cl::Error e) {
        processError(e);
    }
}

void NBody::draw(const Camera* camera) {
    shader.use();
    
    GLint polygon_mode;
    glGetIntegerv(GL_POLYGON_MODE, &polygon_mode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    
    shader.setMat4("PVM", camera->getPVMatrix());
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, body_num);
    glBindVertexArray(0);
    
    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
}
