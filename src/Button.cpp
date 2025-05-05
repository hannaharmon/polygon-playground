#include "Button.h"
#include <GL/glew.h>
#include <iostream>

Button::Button(const glm::vec2& pos, const glm::vec2& size, Tool tool, std::function<void()> onClick, Eigen::Vector4f selectedColor)
    : pos(pos), size(size), tool(tool), onClick(onClick), selectedColor(selectedColor), textureID(0) {
}



void Button::draw(bool active) const {
    if (active) {
        glColor3f(selectedColor.x(), selectedColor.y(), selectedColor.z());
    }
    else {
        glColor3f(0.6f, 0.6f, 0.6f);  // Inactive color
    }

    glBegin(GL_QUADS);
    glVertex2f(pos.x, pos.y);
    glVertex2f(pos.x + size.x, pos.y);
    glVertex2f(pos.x + size.x, pos.y + size.y);
    glVertex2f(pos.x, pos.y + size.y);
    glEnd();

    if (textureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor3f(1, 1, 1); // No tint

        // Draw smaller icon centered inside button with more margin
        float marginScale = 0.25f; // 25% inward padding
        float xMargin = size.x * marginScale;
        float yMargin = size.y * marginScale;

        float x0 = pos.x + xMargin;
        float y0 = pos.y + yMargin;
        float x1 = pos.x + size.x - xMargin;
        float y1 = pos.y + size.y - yMargin;

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
    float xMin = std::min(pos.x, pos.x + size.x);
    float xMax = std::max(pos.x, pos.x + size.x);
    float yMin = std::min(pos.y, pos.y + size.y);
    float yMax = std::max(pos.y, pos.y + size.y);

    bool hovered = worldX >= xMin && worldX <= xMax &&
        worldY >= yMin && worldY <= yMax;

    //std::cout << "isHovered: " << hovered << " (click: " << worldX << "," << worldY
    //    << " vs bounds x[" << xMin << "," << xMax << "] y[" << yMin << "," << yMax << "])\n";

    return hovered;
}


void Button::click() {
    if (onClick) onClick();
}
