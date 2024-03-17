#include "Camera.h"
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include <iostream>
#include <algorithm>

Camera::Camera() : 
    position_(0.0f, 0.0f, 0.0f), 
    front_(0.0f, 0.0f, -1.0f), 
    up_(0.0f, 1.0f, 0.0f), 
    worldUp_(0.0f, 1.0f, 0.0f), 
    right_(1.0f, 0.0f, 0.0f) {
    
}

glm::mat4 Camera::getView() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::getProj(float radians, float width, float height, float near, float far) const {
    return glm::perspective(glm::radians(radians), width / height, near, far);
}

void Camera::onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    return ;
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_W:
            keys_.w_ = true;
            break;
        case GLFW_KEY_S:
            keys_.s_ = true;
            break;
        case GLFW_KEY_D:
            keys_.d_ = true;
            break;
        case GLFW_KEY_A:
            keys_.a_ = true;
            break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_W:
            keys_.w_ = false;
            break;
        case GLFW_KEY_S:
            keys_.s_ = false;
            break;
        case GLFW_KEY_D:
            keys_.d_ = false;
            break;
        case GLFW_KEY_A:
            keys_.a_ = false;
            break;
        }
    }

    processOnKey();
}

void Camera::onMouseMovement(GLFWwindow* window, double xpos, double ypos) {
    // std::cout << xpos << ", " << ypos << std::endl;
    return ;
    static float x = xpos, y = ypos;

    delteX_ = (xpos - x) * deltaTime_ * mouseSpeed_;
    deltaY_ = (ypos - y) * deltaTime_ * mouseSpeed_;

    x = xpos;
    y = ypos;

    yaw_ += delteX_;
    pitch_ += deltaY_;

    pitch_ = std::clamp(pitch_, -89.0f, 90.0f);

    processOnMouseMovement();
}

void Camera::processOnKey() {
    glm::vec3 offset(0.0f, 0.0f, 0.0f);
    
    if (keys_.w_) {
        offset += front_ * moveSpeed_;
    }
    if (keys_.s_) {
        offset -= front_ * moveSpeed_;
    }
    if (keys_.d_) {
        offset += right_ * moveSpeed_;
    }
    if (keys_.a_) {
        offset -= right_ * moveSpeed_;
    }

    position_ += offset * deltaTime_;
}

void Camera::processOnMouseMovement() {
    auto level = glm::angleAxis(glm::radians(-yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
    auto vertical = glm::angleAxis(glm::radians(-pitch_), glm::vec3(1.0f, 0.0f, 0.0f));

    auto rotate = glm::mat3_cast(level) * glm::mat3_cast(vertical);
    front_ = glm::normalize(rotate * beginFront_);

    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right_, front_));
}