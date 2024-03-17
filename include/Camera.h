#pragma once

#include "GLFW/glfw3.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera {
public:
    Camera();

    void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    void onMouseMovement(GLFWwindow* window, double xpos, double ypos);
    
    glm::mat4 getView() const;
    glm::mat4 getProj(float radians, float width, float height, float near, float far) const;

    void setDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }
    
private:;
    void processOnKey();
    void processOnMouseMovement();

    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;

    glm::vec3 beginFront_ = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 worldUp_;
    glm::vec3 right_;

    float yaw_ = 0.0f;
    float pitch_ = 0.0f;

    float deltaTime_;

    float delteX_;
    float deltaY_;

    float moveSpeed_ = 20.0f;
    float mouseSpeed_ = 500.0f;

    struct Keys {
        bool w_;
        bool s_;
        bool d_;
        bool a_;
    };

    Keys keys_{};
};