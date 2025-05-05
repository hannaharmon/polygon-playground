#pragma once
#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include <glm/glm.hpp>
#include "Tool.h"

class Button {
public:
    Button(const glm::vec2& position, const glm::vec2& size, Tool toolType, std::function<void()> onClick);

    void draw(bool active = false) const;
    bool isHovered(float worldX, float worldY) const;
    void click();
    void setTexture(unsigned int tex) { textureID = tex; }
    Tool getTool() const { return toolType; }

private:
    glm::vec2 position;
    glm::vec2 size;
    Tool toolType;
    unsigned int textureID = 0; // OpenGL texture handle
    std::function<void()> onClick;
};

#endif