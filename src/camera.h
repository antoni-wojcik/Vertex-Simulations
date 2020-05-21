//
//  camera.h
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 14/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef camera_h
#define camera_h

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "shader.h"

#define CAMERA_SPEED_SLOW 0.3f
#define CAMERA_SPEED_NORMAL 1.0f
#define CAMERA_SPEED_FAST 5.0f
#define MOUSE_SENSITIVITY 0.2f
#define YAW 0.0f
#define PITCH 0.0f
#define ZOOM_MIN 90.0f
#define ZOOM_MAX 10.0f
#define ZOOM_SPEED 0.5f

#define UP_DIR glm::vec3(0.0f, -1.0f, 0.0f)

enum CameraMovementDirection {
    FORWARD,
    BACK,
    LEFT,
    RIGHT
};

class Camera {
private:
    float fov, aspect;
    float speed;
    
    float yaw, pitch;
    
    glm::vec3 u, v, w;
    glm::vec3 up, position;
    
    glm::mat4 pvMatrix;
    
    inline void updatePVMatrix() {
        pvMatrix = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f) * glm::lookAt(glm::vec3(0.0f), w, v);
    }
    
    inline void update() {
        float rad_pitch = glm::radians(pitch), rad_yaw = glm::radians(yaw);
        w.x = glm::cos(rad_pitch) * glm::sin(rad_yaw);
        w.y = glm::sin(rad_pitch);
        w.z = glm::cos(rad_pitch) * glm::cos(rad_yaw);
        w = normalize(w);
        u = normalize(cross(w, up));
        v = cross(u, w);
        
        updatePVMatrix();
    }
public:
    Camera(float cam_fov = 60.0f, float cam_aspect = 1.0f, const glm::vec3& pos = glm::vec3(0.0f, 0.0f, 0.0f), float y = YAW, float p = PITCH) : fov(cam_fov), aspect(cam_aspect), position(pos), up(UP_DIR), yaw(y), pitch(p), speed(CAMERA_SPEED_SLOW) {
        
        update();
    }
    
    inline void move(CameraMovementDirection dir, float dt) {
        float ds = speed * dt;
        if(dir == FORWARD) position += w * ds;
        else if(dir == BACK) position -= w * ds;
        else if(dir == LEFT) position -= u * ds;
        else if(dir == RIGHT) position += u * ds;
    }
    
    inline void rotate(float x, float y) {
        yaw += x * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
        pitch += y * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
        //constrain the yaw
        if(pitch > 89.0f) pitch = 89.0f;
        else if(pitch < -89.0f) pitch = -89.0f;
        //constranin the pitch
        yaw = fmod(yaw, 360.0f);
        
        update();
    }
    
    inline void setFasterSpeed(bool speed_up) {
        if(speed_up) speed = CAMERA_SPEED_FAST;
        else speed = CAMERA_SPEED_NORMAL;
    }
    
    inline void setSlowerSpeed(bool speed_down) {
        if(speed_down) speed = CAMERA_SPEED_SLOW;
        else speed = CAMERA_SPEED_NORMAL;
    }
    
    inline void zoom(float scroll) {
        fov += scroll * ZOOM_SPEED;
        if(fov < ZOOM_MAX) fov = ZOOM_MAX;
        else if(fov > ZOOM_MIN) fov = ZOOM_MIN;
    }
    
    inline void setSize(float aspect_n) {
        aspect = aspect_n;
        
        updatePVMatrix();
    }
    
    inline glm::mat4 getPVMatrix() const {
        return pvMatrix;
    }
    
    inline glm::vec3 getPosition() const {
        return position;
    }
    
    inline glm::vec3 getNormal() const {
        return w;
    }
};

#endif /* camera_h */
