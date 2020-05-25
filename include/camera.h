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

enum CameraMovementDirection {
    FORWARD,
    BACK,
    LEFT,
    RIGHT
};

class Camera {
private:
    float fov, aspect;
    float yaw, pitch;
    float speed;
    
    glm::vec3 u, v, w;
    glm::vec3 up, position;
    glm::mat4 pvMatrix;
    
    void updatePVMatrix();
    void update();
    
public:
    Camera(float cam_fov, float cam_aspect, const glm::vec3& pos, float y, float p);
    
    void move(CameraMovementDirection dir, float dt);
    void rotate(float x, float y);
    void zoom(float scroll);
    
    void setFasterSpeed(bool speed_up);
    void setSlowerSpeed(bool speed_down);
    void setSize(float aspect_n);
    
    inline glm::mat4 getPVMatrix() const { return pvMatrix; }
    inline glm::vec3 getPosition() const { return position; }
    inline glm::vec3 getNormal() const { return w; }
};

#endif /* camera_h */
