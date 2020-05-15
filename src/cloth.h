//
//  cloth.h
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 14/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef cloth_h
#define cloth_h

// include the standard libraries
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// include the OpenCL library (C++ binding)
#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"
#include "opencl_error.h"

// include the OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm.hpp"

#include "shader.h"
#include "camera.h"


class Cloth {
private:
    struct ClothProperties {
        int size_x, size_y;
        float length; // distance between vertices
        float mass, stiffness, damping;
        float time_step;
        glm::vec3 pos;
        glm::mat4 model_matrix;
        
        ClothProperties(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt) {
            size_x = x;
            size_y = y;
            length = l;
            mass = m;
            stiffness = k;
            damping = b;
            pos = p;
            time_step = dt;
        }
        
        void updateModelMatrix(const Camera& camera) {
            model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, pos - camera.getPosition());
            model_matrix = glm::scale(model_matrix, glm::vec3(1.0f, -1.0f, 1.0f));
        }
    } cloth_prop;
    
    struct GLManager {
        GLuint VBO, VAO, EBO;
        GLsizei indices_num;
    } gl_manager;
    
    struct CLManager {
        cl::Device device;
        cl::Context context;
        cl::Program program;
        size_t buff_size;
    } cl_manager;
    
    cl::Kernel kernel_pos;
    cl::Kernel kernel_vel;
    
    cl::Buffer buff_pos_prev;
    cl::BufferGL buff_pos_next;
    cl::Buffer buff_vel_prev;
    cl::Buffer buff_vel_next;
    
    Shader shader;
    
    
    void processError(cl::Error& e) {
        std::cerr << "ERROR: OpenCL: OTHER: " << e.what() << ": " << e.err() << std::endl;
        if(e.err() == CL_BUILD_PROGRAM_FAILURE) {
            std::cerr << "ERROR: OpenCL: CANNOT BUILD PROGRAM: " << cl_manager.program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl_manager.device) << std::endl;
        } else {
            std::cerr << oclErrorString(e.err()) << "\nUSE:\nhttps://streamhpc.com/blog/2013-04-28/opencl-error-codes\nTO VERIFY ERROR TYPE" << std::endl;
        }
        
        exit(-1);
    }
    
    std::string loadSource(const char* kernel_path) {
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
    
    void buildProgram(const char* kernel_path) {
        // build the program given the source
        
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
        
        cl_manager.device = devices[1]; //choose the graphics card
        std::cout << "SUCCESS: OpenCL: USING A DEVICE: " << cl_manager.device.getInfo<CL_DEVICE_NAME>() << std::endl;
        
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
        
        cl_manager.context = cl::Context(cl_manager.device, properties);
        
        // upload program source
        
        std::string kernel_code = loadSource(kernel_path);
        cl::Program::Sources sources;
        sources.push_back({kernel_code.c_str(), kernel_code.length()});
        
        // build the program
        
        cl_manager.program = cl::Program(cl_manager.context, sources);
        cl_manager.program.build({cl_manager.device});
    }
    
    void createKernels(const char* kernel_pos_name, const char* kernel_vel_name) {
        // create the kernels given the names
        
        kernel_pos = cl::Kernel(cl_manager.program, kernel_pos_name);
        kernel_vel = cl::Kernel(cl_manager.program, kernel_vel_name);
    }
    
    void createGLBuffers() {
        int size_x = cloth_prop.size_x;
        int size_y = cloth_prop.size_y;
        gl_manager.indices_num = (size_x - 1) * (size_y - 1) * 6;
        cl_manager.buff_size = cloth_prop.size_x * cloth_prop.size_y * 3 * sizeof(cl_float);
        
        float* vertices = new float[size_x * size_y * 3];
        unsigned int* indices = new unsigned int[gl_manager.indices_num];
        
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
        
        glGenVertexArrays(1, &gl_manager.VAO);
        glGenBuffers(1, &gl_manager.VBO);
        glGenBuffers(1, &gl_manager.EBO);
        
        glBindVertexArray(gl_manager.VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, gl_manager.VBO);
        glBufferData(GL_ARRAY_BUFFER, cl_manager.buff_size, vertices, GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_manager.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, gl_manager.indices_num * sizeof(float), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        delete [] vertices;
        delete [] indices;
    }
    
    void setConstKernelArgs() {
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
        
        float effective_stiffness = cloth_prop.stiffness * (cloth_prop.size_x * cloth_prop.size_y) / cloth_prop.mass;
        float effective_damping = cloth_prop.damping * (cloth_prop.size_x * cloth_prop.size_y) / cloth_prop.mass;
        
        kernel_vel.setArg(6, effective_stiffness);
        kernel_vel.setArg(7, effective_damping);
        kernel_vel.setArg(8, cloth_prop.time_step * 0.5f);
    }
    
    void createCLBuffers(const cl::Context& context) {
        buff_pos_prev = cl::Buffer(context, CL_MEM_READ_WRITE, cl_manager.buff_size);
        buff_pos_next = cl::BufferGL(context, CL_MEM_READ_WRITE, gl_manager.VBO);
        buff_vel_prev = cl::Buffer(context, CL_MEM_READ_WRITE, cl_manager.buff_size);
        buff_vel_next = cl::Buffer(context, CL_MEM_READ_WRITE, cl_manager.buff_size);
        
        int size = cloth_prop.size_x * cloth_prop.size_x * 3;
        float* vel = new float[size];
        for(int i = 0; i < size; i++) vel[i] = 0.0f;
        
        cl::CommandQueue queue(cl_manager.context, cl_manager.device);
        queue.enqueueWriteBuffer(buff_vel_prev, CL_TRUE, 0, cl_manager.buff_size, vel);
        queue.enqueueCopyBuffer(buff_pos_next, buff_pos_prev, 0, 0, cl_manager.buff_size);
        queue.enqueueBarrierWithWaitList();
        
        setConstKernelArgs();
        
        // set the velocity step from 0 to 1/2
        
        queue.enqueueNDRangeKernel(kernel_vel, cl::NDRange(0, 1), cl::NDRange(size_t(cloth_prop.size_x), size_t(cloth_prop.size_y - 1)), cl::NullRange);
        queue.enqueueCopyBuffer(buff_vel_next, buff_vel_prev, 0, 0, cl_manager.buff_size);
        queue.enqueueBarrierWithWaitList();
        
        kernel_vel.setArg(8, cloth_prop.time_step);
        
        queue.finish();
        
        delete [] vel;
    }
    
public:
    Cloth(int x, int y, float l, float m, float k, float b, const glm::vec3& p, float dt, const char* cloth_vs_path, const char* cloth_gs_path, const char* cloth_fs_path, const char* kernel_path, const char* kernel_pos_name, const char* kernel_vel_name) : cloth_prop(x, y, l, m, k, b, p, dt), shader(cloth_vs_path, cloth_fs_path, cloth_gs_path) {
        try {
            buildProgram(kernel_path);
            createKernels(kernel_pos_name, kernel_vel_name);
            createGLBuffers();
            createCLBuffers(cl_manager.context);
        } catch(cl::Error e) {
            processError(e);
        }
    }
    
    ~Cloth() {
        glDeleteVertexArrays(1, &gl_manager.VAO);
        glDeleteBuffers(1, &gl_manager.VBO);
        glDeleteBuffers(1, &gl_manager.EBO);
    }
    
    void iterate(int steps = 1) {
        try {
            std::vector<cl::Memory> mem_objs;
            mem_objs.push_back(buff_pos_next);
            
            // use the leapfrog algorithm
            
            cl::CommandQueue queue(cl_manager.context, cl_manager.device);
            
            for(int i = 0; i < steps; i++) {
            
                // calculate new position
            
                // make sure the OpenGL has released the buffer
                queue.enqueueAcquireGLObjects(&mem_objs);
                queue.enqueueNDRangeKernel(kernel_pos, cl::NDRange(0, 1), cl::NDRange(size_t(cloth_prop.size_x), size_t(cloth_prop.size_y - 1)), cl::NullRange);
                queue.enqueueCopyBuffer(buff_pos_next, buff_pos_prev, 0, 0, cl_manager.buff_size);
                queue.enqueueReleaseGLObjects(&mem_objs);
                queue.enqueueBarrierWithWaitList();
                
                // calculate new velocity
                
                queue.enqueueNDRangeKernel(kernel_vel, cl::NDRange(0, 1), cl::NDRange(size_t(cloth_prop.size_x), size_t(cloth_prop.size_y - 1)), cl::NullRange);
                queue.enqueueCopyBuffer(buff_vel_next, buff_vel_prev, 0, 0, cl_manager.buff_size);
                queue.enqueueBarrierWithWaitList();
            }
            
            queue.finish();
        } catch(cl::Error e) {
            processError(e);
        }
    }
    
    void draw(const Camera& camera) {
        shader.use();
        
        cloth_prop.updateModelMatrix(camera);
        
        shader.setMat4("PVM", camera.getPVMatrix() * cloth_prop.model_matrix);
        shader.setMat3("M_normals", glm::mat3(glm::transpose(glm::inverse(cloth_prop.model_matrix))));
        
        glBindVertexArray(gl_manager.VAO);
        glDrawElements(GL_TRIANGLES, gl_manager.indices_num, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

#endif /* cloth_h */
