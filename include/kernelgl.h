//
//  kernelgl.h
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 24/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef kernelgl_h
#define kernelgl_h

// include the OpenCL library (C++ binding)
#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"
#include "opencl_error.h"

// include project libraries
#include "camera.h"

class KernelGL {
private:
    std::string loadSource(const char* kernel_path);
    
protected:
    cl::Device device;
    cl::Context context;
    cl::Program program;
    
    void processError(cl::Error& e);
    
    void buildProgram(const char* kernel_path);
    
public:
    virtual ~KernelGL() {}
    void initialiseOpenCL();
    
    virtual void iterate(int steps) = 0;
    virtual void draw(const Camera* const) = 0;
};

#endif /* kernelgl_h */
