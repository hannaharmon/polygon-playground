#include "Button.h"
#include <GL/glew.h>
#include <iostream>

Button::Button(const glm::vec2& position, const glm::vec2& size, const Tool toolType, std::function<void()> onClick)
    : position(position), size(size), toolType(toolType), onClick(onClick) {
}


void Button::draw(bool active) const {
    if (active) {
        glColor3f(0.3f, 0.7f, 1.0f);  // Active tool color
    }
    else {
        glColor3f(0.6f, 0.6f, 0.6f);  // Inactive color
    }

    glBegin(GL_QUADS);
    glVertex2f(position.x, position.y);
    glVertex2f(position.x + size.x, position.y);
    glVertex2f(position.x + size.x, position.y + size.y);
    glVertex2f(position.x, position.y + size.y);
    glEnd();

    if (textureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor3f(1, 1, 1); // No tint

        // Draw smaller icon centered inside button with more margin
        float marginScale = 0.25f; // 25% inward padding
        float xMargin = size.x * marginScale;
        float yMargin = size.y * marginScale;

        float x0 = position.x + xMargin;
        float y0 = position.y + yMargin;
        float x1 = position.x + size.x - xMargin;
        float y1 = position.y + size.y - yMargin;

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(x0, y0);
        glTexCoord2f(1, 0); glVertex2f(x1, y0);
        glTexCoord2f(1, 1); glVertex2f(x1, y1);
        glTexCoord2f(0, 1); glVertex2f(x0, y1);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }


}

bool Button::isHovered(float worldX, float worldY) const {
    float xMin = std::min(position.x, position.x + size.x);
    float xMax = std::max(position.x, position.x + size.x);
    float yMin = std::min(position.y, position.y + size.y);
    float yMax = std::max(position.y, position.y + size.y);

    bool hovered = worldX >= xMin && worldX <= xMax &&
        worldY >= yMin && worldY <= yMax;

    //std::cout << "isHovered: " << hovered << " (click: " << worldX << "," << worldY
    //    << " vs bounds x[" << xMin << "," << xMax << "] y[" << yMin << "," << yMax << "])\n";

    return hovered;
}


void Button::click() {
    if (onClick) onClick();
}
