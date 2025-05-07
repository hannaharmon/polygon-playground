#pragma once
#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include <glm/glm.hpp>
#include <Eigen/Dense>
#include "Tool.h"

class Button {
public:
    Button(const glm::vec2& pos, const glm::vec2& size, Tool tool, std::function<void()> onClick, Eigen::Vector4f selectedColor);

    void draw(bool active = false) const;
    bool isHovered(float worldX, float worldY) const;
    void click();
    void setTexture(unsigned int tex) { textureID = tex; }
    Tool getTool() const { return tool; }
    void setPosition(const glm::vec2& newPos);


private:
    glm::vec2 pos;
    glm::vec2 size;
    Eigen::Vector4f selectedColor;
    Tool tool;
    unsigned int textureID = 0; // OpenGL texture handle
    std::function<void()> onClick;
};

#endif