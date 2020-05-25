//
//  camera.cpp
//  Vertex Simulations
//
//  Created by Antoni Wójcik on 25/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include "camera.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#define CAMERA_SPEED_SLOW 0.3f
#define CAMERA_SPEED_NORMAL 1.0f
#define CAMERA_SPEED_FAST 5.0f
#define MOUSE_SENSITIVITY 0.2f
#define ZOOM_MIN 90.0f
#define ZOOM_MAX 10.0f
#define ZOOM_SPEED 0.5f
#define UP_DIR glm::vec3(0.0f, -1.0f, 0.0f)

Camera::Camera(float cam_fov, float cam_aspect, const glm::vec3& pos, float y, float p) : fov(cam_fov), aspect(cam_aspect), position(pos), up(UP_DIR), yaw(y), pitch(p), speed(CAMERA_SPEED_SLOW) {
    
    update();
}

void Camera::updatePVMatrix() {
    pvMatrix = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f) * glm::lookAt(glm::vec3(0.0f), w, v);
}

void Camera::update() {
    float rad_pitch = glm::radians(pitch), rad_yaw = glm::radians(yaw);
    w.x = glm::cos(rad_pitch) * glm::sin(rad_yaw);
    w.y = glm::sin(rad_pitch);
    w.z = glm::cos(rad_pitch) * glm::cos(rad_yaw);
    w = normalize(w);
    u = normalize(cross(w, up));
    v = cross(u, w);
    
    updatePVMatrix();
}

void Camera::move(CameraMovementDirection dir, float dt) {
    float ds = speed * dt;
    if(dir == FORWARD) position += w * ds;
    else if(dir == BACK) position -= w * ds;
    else if(dir == LEFT) position -= u * ds;
    else if(dir == RIGHT) position += u * ds;
}

void Camera::rotate(float x, float y) {
    yaw += x * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
    pitch += y * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
    //constrain the yaw
    if(pitch > 89.0f) pitch = 89.0f;
    else if(pitch < -89.0f) pitch = -89.0f;
    //constranin the pitch
    yaw = fmod(yaw, 360.0f);
    
    update();
}

void Camera::zoom(float scroll) {
    fov += scroll * ZOOM_SPEED;
    if(fov < ZOOM_MAX) fov = ZOOM_MAX;
    else if(fov > ZOOM_MIN) fov = ZOOM_MIN;
    
    updatePVMatrix();
}

void Camera::setFasterSpeed(bool speed_up) {
    if(speed_up) speed = CAMERA_SPEED_FAST;
    else speed = CAMERA_SPEED_NORMAL;
}

void Camera::setSlowerSpeed(bool speed_down) {
    if(speed_down) speed = CAMERA_SPEED_SLOW;
    else speed = CAMERA_SPEED_NORMAL;
}

void Camera::setSize(float aspect_n) {
    aspect = aspect_n;
    
    updatePVMatrix();
}

