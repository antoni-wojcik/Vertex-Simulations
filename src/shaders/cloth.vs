#version 410 core
layout (location = 0) in vec3 a_pos;

uniform mat4 PVM;

void main() {
    vec4 pos4 = vec4(a_pos, 1.0f);
    
    gl_Position = PVM * pos4;
}

