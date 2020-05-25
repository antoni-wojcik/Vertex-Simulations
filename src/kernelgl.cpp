//
//  kernelgl.cpp
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 24/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include "kernelgl.h"

#include <OpenGL/OpenGL.h>

// include the standard libraries
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

KernelGL::KernelGL(const char* kernel_path) {
    try {
        initialiseOpenCL();
        buildProgram(kernel_path);
    } catch(cl::Error e) {
        processError(e);
    }
}

std::string KernelGL::loadSource(const char* kernel_path) {
    std::string kernel_code;
    std::ifstream kernel_file;
    kernel_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        kernel_file.open(kernel_path);
        std::ostringstream kernel_stream;
        kernel_stream << kernel_file.rdbuf();
        kernel_file.close();
        kernel_code = kernel_stream.str();
    } catch(std::ifstream::failure e) {
        std::cerr << "ERROR: OpenCL KERNEL: CANNOT READ KERNEL CODE" << std::endl;
        exit(-1); //stop executing the program with the error code -1;
    }
    return kernel_code;
}

void KernelGL::processError(cl::Error& e) {
    std::cerr << "ERROR: OpenCL: OTHER: " << e.what() << ": " << e.err() << std::endl;
    if(e.err() == CL_BUILD_PROGRAM_FAILURE) {
        std::cerr << "ERROR: OpenCL: CANNOT BUILD PROGRAM: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(KernelGL::device) << std::endl;
    } else {
        std::cerr << oclErrorString(e.err()) << "\nUSE:\nhttps://streamhpc.com/blog/2013-04-28/opencl-error-codes\nTO VERIFY ERROR TYPE" << std::endl;
    }
    
    exit(-1);
}

void KernelGL::initialiseOpenCL() {
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;
    
    // find platform
    
    cl::Platform::get(&platforms);
    if(platforms.size() == 0) {
        std::cerr << "ERROR: OpenCL: NO PLATFORMS FOUND" << std::endl;;
    }
    
    // find device
    
    platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if(devices.size() == 0) {
        std::cerr << "ERROR: OpenCL: NO DEVICES FOUND" << std::endl;
    }
    
    device = devices[1]; //choose the graphics card
    std::cout << "SUCCESS: OpenCL: USING A DEVICE: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
    
    // create shared context between OpenCL and OpenGL - therefore no communication via host needed!
    
    CGLContextObj CGLGetCurrentContext(void);
    CGLShareGroupObj CGLGetShareGroup(CGLContextObj);
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties) kCGLShareGroup,
        0
    };
    
    context = cl::Context(device, properties);
}

void KernelGL::buildProgram(const char* kernel_path) {
    // upload program source
    
    std::string kernel_code = loadSource(kernel_path);
    cl::Program::Sources sources;
    sources.push_back({kernel_code.c_str(), kernel_code.length()});
    
    // build the program
    
    program = cl::Program(context, sources);
    program.build({device});
}
