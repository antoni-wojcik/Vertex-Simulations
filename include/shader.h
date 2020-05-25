// Created by Joey de Vries under the CC BY-NC 4.0 license: https://creativecommons.org/licenses/by-nc/4.0/legalcode.
// Joey de Vries's Twitter: https://twitter.com/JoeyDeVriez.
// Source code taken from the following OpenGL tutorial: https://learnopengl.com/Getting-started/Shaders
// More on https://learnopengl.com/About
//

#ifndef shader_h
#define shader_h

#include <string>
#include "glm.hpp"

class Shader {
public:
    unsigned int ID;
    
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    
    void use();
    
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    
private:
    void checkCompileErrors(unsigned int shader, std::string type);
    
};

#endif /* shader_h */
