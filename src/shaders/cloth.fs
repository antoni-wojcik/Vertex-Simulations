#version 410 core
out vec4 frag_color;

in vec3 normal;

const vec3 light_dir = normalize(vec3(0, 0, 1.0f));
const vec3 light_col = vec3(0.980f, 0.658f, 0.776f);

void main() {
    float b = abs(dot(normal, light_dir));
    frag_color = vec4(vec3(b) * light_col, 1.0f);
}
