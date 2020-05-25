//
//  nbody.h
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 25/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef nbody_h
#define nbody_h

#include "kernelgl.h"
#include "shader.h"

class NBody : public KernelGL {
private:
    int grid_num;
    GLsizei body_num;
    float body_mass;
    
    GLuint VBO, VAO;
    
    Shader shader;
    
    cl::Kernel kernel_pos;
    cl::Kernel kernel_vel;
    cl::Kernel kernel_dens;
    cl::Kernel kernel_pot;
    cl::Kernel kernel_acc;
    
    cl::Buffer buff_pos_0;
    cl::BufferGL buff_pos_1;
    cl::Buffer buff_vel_0;
    cl::Buffer buff_vel_1;
    cl::Buffer buff_acc;
    cl::Buffer buff_dens; // stores density distribution
    cl::Buffer buff_pot; // stores potential
    cl::Buffer buff_FFT_h; // stores data to speed up FFT by convolution thm.
    
    size_t buff_v_size, buff_s_size; // vector buffer size; scalar buffer size
    
    void createGLBuffers();
    void createCLBuffers();
    void createKernels();
    void setConstKernelArgs();
    
public:
    NBody(int g, int n, float m, const char* vs_path, const char* fs_path, const char* kernel_path);
    ~NBody();
    
    virtual void iterate(int steps = 1);
    virtual void draw(const Camera* camera);
};

#endif /* nbody_h */
