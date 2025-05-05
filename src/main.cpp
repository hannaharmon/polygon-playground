#include <iostream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <Eigen/Dense>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SceneManager.h"
#include "PolygonFactory.h"
#include "Polygon.h"
#include "Particle.h"
#include "Button.h"
#include "Tool.h"

std::vector<Button> buttons;

using namespace std;
using namespace Eigen;

// Simulation parameters
const double timeStep = 1.0 / 60.0;
const int springIters = 10;
const int collisionIters = 10;
const Vector3d gravity(0.0, -9.8, 0.0);
const double groundY = -1.0;
const double damping = 0.98;
const double flickForceScale = 10;

// Globals
SceneManager sceneManager;
vector<shared_ptr<Polygon>> polygons;
GLFWwindow* window;

bool uiHovered = false;

// Camera
Eigen::Vector2f cameraPosition(0.0f, 0.0f);
float cameraZoom = 1.0f;

bool panning = false;
Eigen::Vector2f panStartWorld;
Eigen::Vector2f panStartMouse;


// Colors
const Eigen::Vector4f flickOutlineColor(1.0f, 1.0f, 0.0f, 1.0f); // yellow
const Eigen::Vector4f grabOutlineColor(0.0f, 1.0f, 0.0f, 1.0f);  // green
const Eigen::Vector4f selectedOutlineColor(0.3f, 0.5f, 1.0f, 1.0f);
const Eigen::Vector4f eraserHoverOutlineColor(1.0f, 0.2f, 0.2f, 1.0f);  // strong red

// Line colors
const Eigen::Vector3f flickLineColor(1.0f, 1.0f, 0.0f);
const Eigen::Vector3f grabLineColor(0.0f, 1.0f, 0.0f);

// Selection box colors
const Eigen::Vector4f selectionBoxFill(0.3f, 0.5f, 1.0f, 0.2f);
const Eigen::Vector3f selectionBoxOutline(0.3f, 0.5f, 1.0f);

Tool currentTool = Tool::Flick;
Tool previousTool = Tool::None;
bool isQuickSwapping = false;

Eigen::Vector2f normalizedOffset;

// Flick globals
bool flickActive = false;
Eigen::Vector2f flickStartCenterOffset;
Eigen::Vector2f flickCurrent;

// Grab globals
bool grabActive = false;
Eigen::Vector2f grabStartCenterOffset;
Eigen::Vector2f grabCurrent;

// Eraser globals
std::unordered_map<std::shared_ptr<Polygon>, int> eraserCountdowns;
const int eraserDelayFrames = 3;

// Pencil globals
double lastPencilTime = 0.0;
const double toolRepeatDelay = 0.2;  // seconds between actions
Eigen::Vector2f pencilMousePos;
float pencilSizeX = 0.3f;
float pencilSizeY = 0.3f;
int pencilSides = 4;
float pencilRotation = 0.0f;

// Selection globals
std::vector<std::shared_ptr<Polygon>> selectedPolygons;
bool selecting = false;
Eigen::Vector2f selectStart, selectEnd;

// Cursors
GLFWcursor* arrowCursor;
GLFWcursor* handCursor;
GLFWcursor* crosshairCursor;
GLFWcursor* ibeamCursor;

unsigned int LoadTexture(const std::string& directory, const std::string& filename) {
    std::string fullPath = directory + "/" + filename;

    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        // try to load placeholder
        std::string fullPath = directory + "/" + "star.png";
        data = stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    }
    if (!data) {
        std::cerr << "Failed to load image: " << fullPath << std::endl;
        return 0;
    }

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    float aspect = width / (float)height;
    float viewHeight = 2.0f / cameraZoom;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspect >= 1.0f) {
        glOrtho(-viewHeight * aspect + cameraPosition.x(), viewHeight * aspect + cameraPosition.x(),
            -viewHeight + cameraPosition.y(), viewHeight + cameraPosition.y(),
            -1.0f, 1.0f);
    }
    else {
        glOrtho(-viewHeight + cameraPosition.x(), viewHeight + cameraPosition.x(),
            -viewHeight / aspect + cameraPosition.y(), viewHeight / aspect + cameraPosition.y(),
            -1.0f, 1.0f);
    }

    glMatrixMode(GL_MODELVIEW);
}

void setScreenSpaceProjection(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1.0f, 1.0f);  // Top-left origin

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void updateProjection(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = width / (float)height;
    float viewHeight = 2.0f / cameraZoom;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspect >= 1.0f) {
        glOrtho(-viewHeight * aspect + cameraPosition.x(), viewHeight * aspect + cameraPosition.x(),
            -viewHeight + cameraPosition.y(), viewHeight + cameraPosition.y(),
            -1.0f, 1.0f);
    }
    else {
        glOrtho(-viewHeight + cameraPosition.x(), viewHeight + cameraPosition.x(),
            -viewHeight / aspect + cameraPosition.y(), viewHeight / aspect + cameraPosition.y(),
            -1.0f, 1.0f);
    }

    glMatrixMode(GL_MODELVIEW);
}



Eigen::Vector2f screenToWorld(GLFWwindow* window, double sx, double sy) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  // use framebuffer size, not window size

    // Flip Y because OpenGL origin is bottom-left
    float ndcX = (float(sx) / width) * 2.0f - 1.0f;
    float ndcY = 1.0f - (float(sy) / height) * 2.0f;

    float aspect = width / (float)height;
    float viewHeight = 2.0f / cameraZoom;

    float worldX, worldY;

    if (aspect >= 1.0f) {
        float halfWidth = viewHeight * aspect;
        float left = -halfWidth + cameraPosition.x();
        float right = halfWidth + cameraPosition.x();
        worldX = ((ndcX + 1.0f) / 2.0f) * (right - left) + left;

        float bottom = -viewHeight + cameraPosition.y();
        float top = viewHeight + cameraPosition.y();
        worldY = ((ndcY + 1.0f) / 2.0f) * (top - bottom) + bottom;
    }
    else {
        float halfHeight = viewHeight / aspect;
        float left = -viewHeight + cameraPosition.x();
        float right = viewHeight + cameraPosition.x();
        worldX = ((ndcX + 1.0f) / 2.0f) * (right - left) + left;

        float bottom = -halfHeight + cameraPosition.y();
        float top = halfHeight + cameraPosition.y();
        worldY = ((ndcY + 1.0f) / 2.0f) * (top - bottom) + bottom;
    }

    return Eigen::Vector2f(worldX, worldY);
}



bool isClickOnSelectedPolygon(const Eigen::Vector2f& click) {
    for (const auto& poly : selectedPolygons) {
        if (poly->containsPoint(click, 0.05f)) return true;
    }
    return false;
}

void clearSelection() {
    for (auto& poly : selectedPolygons) {
        poly->outlineColor = poly->defaultOutlineColor;
    }
    selectedPolygons.clear();
}

std::shared_ptr<Polygon> getClickedSelectedPolygon(const Eigen::Vector2f& click) {
    for (const auto& poly : selectedPolygons) {
        if (poly->containsPoint(click, 0.05f)) return poly;
    }
    return nullptr;
}

void updateEraserHoverOutlines(const Eigen::Vector2f& world) {
    std::shared_ptr<Polygon> hovered = nullptr;

    // Find the hovered polygon
    for (auto& poly : polygons) {
        if (poly->containsPoint(world, 0.05f)) {
            hovered = poly;
            break;
        }
    }

    bool hoveredIsSelected = hovered && std::find(selectedPolygons.begin(), selectedPolygons.end(), hovered) != selectedPolygons.end();

    for (auto& poly : polygons) {
        bool isSelected = std::find(selectedPolygons.begin(), selectedPolygons.end(), poly) != selectedPolygons.end();

        if (hoveredIsSelected && isSelected) {
            poly->outlineColor = eraserHoverOutlineColor;  // all selected turn red
        }
        else if (hovered == poly && !isSelected) {
            poly->outlineColor = eraserHoverOutlineColor;  // single hovered deselected turns red
        }
        else if (isSelected) {
            poly->outlineColor = selectedOutlineColor;
        }
        else {
            poly->outlineColor = poly->defaultOutlineColor;
        }
    }
}


void switchTool(Tool newTool) {
    if (newTool == currentTool) return;

    // Cleanup from Eraser hover effect
    if (currentTool == Tool::Eraser) {
        for (auto& poly : polygons) {
            if (std::find(selectedPolygons.begin(), selectedPolygons.end(), poly) != selectedPolygons.end()) {
                poly->outlineColor = selectedOutlineColor;
            }
            else {
                poly->outlineColor = poly->defaultOutlineColor;
            }
        }
    }

    currentTool = newTool;

    if (window) {  // make sure window is valid
        switch (newTool) {
        case Tool::View:
            glfwSetCursor(window, handCursor);
            break;
        case Tool::Pencil:
        case Tool::Select:
            glfwSetCursor(window, crosshairCursor);
            break;
        case Tool::Eraser:
        case Tool::Flick:
        case Tool::Grab:
            glfwSetCursor(window, arrowCursor);
            break;
        default:
            glfwSetCursor(window, nullptr);  // fallback to system default
            break;
        }
    }

    if (newTool == Tool::Eraser) {
        // Re-run hover logic in case cursor is already over a polygon
        double sx, sy;
        glfwGetCursorPos(window, &sx, &sy);
        Eigen::Vector2f worldPos = screenToWorld(window, sx, sy);
        updateEraserHoverOutlines(worldPos);
    }

}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    switch (codepoint) {
    case 'v':
    case 'V':
        switchTool(Tool::View);
        break;
    case 'f':
    case 'F':
        switchTool(Tool::Flick);
        break;
    case 'g':
    case 'G':
        switchTool(Tool::Grab);
        break;
    case 's':
    case 'S':
        switchTool(Tool::Select);
        break;
    case 'p':
    case 'P':
        switchTool(Tool::Pencil);
        break;
    case 'e':
    case 'E':
        switchTool(Tool::Eraser);
        break;
    }
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double sx, sy;
    glfwGetCursorPos(window, &sx, &sy);
    Eigen::Vector2f worldClick = screenToWorld(window, sx, sy);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Convert to float screen-space (no need to map to world space)
        float mouseX = static_cast<float>(sx);
        float mouseY = static_cast<float>(sy);

        for (auto& b : buttons) {
            if (b.isHovered(mouseX, mouseY)) {
                b.click();
                uiHovered = true;
                return; // Stop here if a button was clicked
            }
        }
    }

    // If not button press, try to use current tool
    switch (currentTool) {

    case Tool::View:
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            panning = true;

            double sx, sy;
            glfwGetCursorPos(window, &sx, &sy);
            panStartMouse = Eigen::Vector2f(sx, sy);
            panStartWorld = cameraPosition;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            panning = false;
        }
        break;

    case Tool::Flick:
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                std::shared_ptr<Polygon> clickedPolygon = getClickedSelectedPolygon(worldClick);

                if (!selectedPolygons.empty() && !clickedPolygon) {
                    clearSelection();
                    return;
                }

                if (selectedPolygons.empty()) {
                    for (auto& poly : polygons) {
                        if (poly->containsPoint(worldClick, 0.05f)) {
                            selectedPolygons = { poly };
                            clickedPolygon = poly;
                            break;
                        }
                    }
                }

                if (clickedPolygon) {
                    Eigen::Vector2f rawOffset = worldClick - clickedPolygon->getCenter();
                    float baseRadius = clickedPolygon->getBoundingRadius();
                    normalizedOffset = rawOffset / baseRadius;
                    flickCurrent = worldClick;
                    flickActive = true;

                    for (auto& p : selectedPolygons) {
                        p->outlineColor = flickOutlineColor;
                    }
                }
            }
            else if (action == GLFW_RELEASE && flickActive) {
                flickActive = false;
                for (auto& poly : selectedPolygons) {
                    float currentRadius = poly->getBoundingRadius();
                    Eigen::Vector2f adjustedOffset = normalizedOffset * currentRadius;
                    Eigen::Vector2f start = poly->getCenter() + adjustedOffset;
                    Eigen::Vector2f dir = start - flickCurrent;

                    if (dir.norm() > 1e-4) {
                        poly->applyImpulseAt(start, dir * flickForceScale);
                    }
                    poly->outlineColor = poly->defaultOutlineColor;
                }
                selectedPolygons.clear();
            }
        }
        break;


    case Tool::Grab:
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                std::shared_ptr<Polygon> clickedPolygon = getClickedSelectedPolygon(worldClick);

                if (!selectedPolygons.empty() && !clickedPolygon) {
                    clearSelection();
                    return;
                }

                if (selectedPolygons.empty()) {
                    for (auto& poly : polygons) {
                        if (poly->containsPoint(worldClick, 0.05f)) {
                            selectedPolygons = { poly };
                            clickedPolygon = poly;
                            break;
                        }
                    }
                }

                if (clickedPolygon) {
                    Eigen::Vector2f rawOffset = worldClick - clickedPolygon->getCenter();
                    float baseRadius = clickedPolygon->getBoundingRadius();
                    normalizedOffset = rawOffset / baseRadius;
                    grabCurrent = worldClick;
                    grabActive = true;

                    for (auto& p : selectedPolygons) {
                        p->outlineColor = grabOutlineColor;
                    }
                }
            }

            else if (action == GLFW_RELEASE && grabActive) {
                grabActive = false;
                for (auto& poly : selectedPolygons) {
                    poly->outlineColor = poly->defaultOutlineColor;
                }
                selectedPolygons.clear();
            }
        }
        break;


    case Tool::Eraser:
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            std::shared_ptr<Polygon> clickedPolygon = nullptr;

            for (auto& poly : polygons) {
                if (poly->containsPoint(worldClick, 0.05f)) {
                    clickedPolygon = poly;
                    break;
                }
            }

            if (clickedPolygon) {
                bool isSelected = std::find(selectedPolygons.begin(), selectedPolygons.end(), clickedPolygon) != selectedPolygons.end();

                if (isSelected) {
                    // If selected, delete all selected polygons
                    for (const auto& poly : selectedPolygons) {
                        polygons.erase(std::remove(polygons.begin(), polygons.end(), poly), polygons.end());
                    }
                    selectedPolygons.clear();
                }
                else {
                    // If not selected, delete just the clicked one and clear selection
                    polygons.erase(std::remove(polygons.begin(), polygons.end(), clickedPolygon), polygons.end());
                    clearSelection();
                }
            }
            else {
                // Clicked in empty space: deselect
                clearSelection();
            }
        }
        break;

    case Tool::Pencil:
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            polygons.push_back(
                PolygonFactory::CreateRegularPolygon(
                    Vector3d(pencilMousePos.x(), pencilMousePos.y(), 0.0),
                    pencilSides,
                    pencilSizeX, pencilSizeY,
                    pencilRotation
                )
            );
            lastPencilTime = glfwGetTime(); // Prevent immediate double-spawn
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            pencilSides = (pencilSides % 8) + 3; // cycle 3–10 sides
        }
        break;

    case Tool::Select:
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            selecting = true;
            selectStart = worldClick;
            selectEnd = selectStart;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && selecting) {
            selecting = false;
            selectEnd = worldClick;

            // Build bounding box
            float xMin = std::min(selectStart.x(), selectEnd.x());
            float xMax = std::max(selectStart.x(), selectEnd.x());
            float yMin = std::min(selectStart.y(), selectEnd.y());
            float yMax = std::max(selectStart.y(), selectEnd.y());

            for (auto& poly : selectedPolygons) {
                poly->outlineColor = poly->defaultOutlineColor;
            }
            selectedPolygons.clear();
            for (auto& poly : polygons) {
                bool intersects = false;
                for (auto& particle : poly->particles) {
                    Eigen::Vector2f pos(particle->x.x(), particle->x.y());
                    if (pos.x() >= xMin && pos.x() <= xMax &&
                        pos.y() >= yMin && pos.y() <= yMax) {
                        intersects = true;
                        break;
                    }
                }
                if (intersects) {
                    selectedPolygons.push_back(poly);
                    poly->outlineColor = selectedOutlineColor;
                }
            }

        }
        break;


    default:
        break;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float scale = (yoffset > 0) ? 1.1f : 0.9f;

    if (currentTool == Tool::Pencil) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
            pencilRotation += (yoffset > 0 ? 1 : -1) * 0.1f;
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            pencilSizeX *= scale;
            pencilSizeX = std::clamp(pencilSizeX, 0.05f, 1.5f);
        }
        else {
            pencilSizeY *= scale;
            pencilSizeY = std::clamp(pencilSizeY, 0.05f, 1.5f);
        }
    }

    if (currentTool == Tool::View) {
        double sx, sy;
        glfwGetCursorPos(window, &sx, &sy);
        Eigen::Vector2f worldBefore = screenToWorld(window, sx, sy);

        float zoomFactor = (yoffset > 0) ? 1.1f : 1.0f / 1.1f;
        cameraZoom *= zoomFactor;

        Eigen::Vector2f worldAfter = screenToWorld(window, sx, sy);
        cameraPosition += worldBefore - worldAfter;

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        updateProjection(window);

    }

}


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Eigen::Vector2f world = screenToWorld(window, xpos, ypos);

    if (currentTool == Tool::View && panning) {
        double sx, sy;
        glfwGetCursorPos(window, &sx, &sy);
        Eigen::Vector2f newWorld = screenToWorld(window, sx, sy);
        Eigen::Vector2f startWorld = screenToWorld(window, panStartMouse.x(), panStartMouse.y());
        cameraPosition = panStartWorld + (startWorld - newWorld);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        updateProjection(window);
    }

    if (flickActive) {
        flickCurrent = world;
    }
    if (grabActive) {
        grabCurrent = world;
    }
    if (currentTool == Tool::Select && selecting) {
        selectEnd = screenToWorld(window, xpos, ypos);
    }
    pencilMousePos = screenToWorld(window, xpos, ypos);
}

void LoadScene(int key) {
	sceneManager.LoadScene(key);
	polygons = sceneManager.GetPolygons();
    selectedPolygons.clear();
}

void initScenes() {

    sceneManager.RegisterScene(1, []() {
        return PolygonFactory::CreateWall(Vector3d(-1.2, -.8, 0), 3, 3, 0.4, 0.4);
        });

    sceneManager.RegisterScene(2, []() {
        return PolygonFactory::CreateStackedRectangles(Vector3d(0, -.8, 0), 4, 0.2, 0.4);
        });

    sceneManager.RegisterScene(3, []() {
        return PolygonFactory::CreateGridOfPolygons(Vector3d(0, 0.5, 0), 2, 3, 6, 0.5, 0.5, 0.1, 0.1);
        });

    // Load the first scene by default
	LoadScene(1);
}

void initButtons() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);

    struct ToolButtonData {
        Tool tool;
        const char* iconFile;
        Eigen::Vector4f selectedColor;
    };

    std::vector<ToolButtonData> toolButtons = {
    { Tool::Eraser, "eraser.png",  eraserHoverOutlineColor },
    { Tool::Pencil, "pencil.png", {1.0f, 0.55f, 0.1f, 1.0f} },
    { Tool::Flick,  "flick.png",   flickOutlineColor },
    { Tool::Grab,   "grab.png",    grabOutlineColor },
    { Tool::Select, "select.png",  selectedOutlineColor },
    { Tool::View, "view.png", {0.7f, 0.3f, 0.9f, 1.0f} },
    };

    const int btnSize = 40;
    const int spacing = 10;
    int x = 10;

    for (const auto& tb : toolButtons) {
        glm::vec2 pos(x, 10);  // 10px from top (screen-space)
        glm::vec2 size(btnSize, btnSize);

        Button button(pos, size, tb.tool, [tool = tb.tool]() {
            switchTool(tool);
            }, tb.selectedColor);

        std::string directory = "../assets/icons";
        unsigned int texture = LoadTexture(directory, std::string(tb.iconFile));
        button.setTexture(texture);

        buttons.emplace_back(std::move(button));

        x += btnSize + spacing;
    }
}


void display(GLFWwindow* window) {
    for (auto& poly : polygons) {
        poly->step(
            timeStep,
            springIters,                
            collisionIters,               
            groundY,
            polygons,
            gravity,
            damping
        );
    }

    updateProjection(window);
    glLoadIdentity(); // Reset modelview

    glClear(GL_COLOR_BUFFER_BIT);
    for (auto& poly : polygons) {
        poly->draw();
    }

    if (currentTool == Tool::Pencil) {
        auto ghost = PolygonFactory::CreateRegularPolygon(
            Vector3d(pencilMousePos.x(), pencilMousePos.y(), 0.0),
            pencilSides,
            pencilSizeX, pencilSizeY,
            pencilRotation
        );
        ghost->fillColor.w() = 0.3f;
        ghost->outlineColor.w() = 0.6f;
        
        ghost->draw();
    }

    if (currentTool == Tool::Select && selecting) {
        glColor4f(selectionBoxFill.x(), selectionBoxFill.y(), selectionBoxFill.z(), selectionBoxFill.w());
        glBegin(GL_QUADS);
        glVertex2f(selectStart.x(), selectStart.y());
        glVertex2f(selectEnd.x(), selectStart.y());
        glVertex2f(selectEnd.x(), selectEnd.y());
        glVertex2f(selectStart.x(), selectEnd.y());
        glEnd();

        glColor3f(selectionBoxOutline.x(), selectionBoxOutline.y(), selectionBoxOutline.z());
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex2f(selectStart.x(), selectStart.y());
        glVertex2f(selectEnd.x(), selectStart.y());
        glVertex2f(selectEnd.x(), selectEnd.y());
        glVertex2f(selectStart.x(), selectEnd.y());
        glEnd();
    }

    if (flickActive) {
        glLineWidth(3.0f);
        glColor3f(flickLineColor.x(), flickLineColor.y(), flickLineColor.z());
        glBegin(GL_LINES);
        for (auto& poly : selectedPolygons) {
            float currentRadius = poly->getBoundingRadius();
            Eigen::Vector2f adjustedOffset = normalizedOffset * currentRadius;
            Eigen::Vector2f start = poly->getCenter() + adjustedOffset;
            glVertex2f(start.x(), start.y());
            glVertex2f(flickCurrent.x(), flickCurrent.y());
        }
        glEnd();
    }


    else if (grabActive) {
        glLineWidth(3.0f);
        glColor3f(grabLineColor.x(), grabLineColor.y(), grabLineColor.z());
        glBegin(GL_LINES);
        for (auto& poly : selectedPolygons) {
            float currentRadius = poly->getBoundingRadius();
            Eigen::Vector2f adjustedOffset = normalizedOffset * currentRadius;
            Eigen::Vector2f start = poly->getCenter() + adjustedOffset;
            glVertex2f(start.x(), start.y());
            glVertex2f(grabCurrent.x(), grabCurrent.y());
        }
        glEnd();

        // Apply force
        for (auto& poly : selectedPolygons) {
            float currentRadius = poly->getBoundingRadius();
            Eigen::Vector2f adjustedOffset = normalizedOffset * currentRadius;
            Eigen::Vector2f grabStart = poly->getCenter() + adjustedOffset;
            Eigen::Vector2f pull = grabCurrent - grabStart;

            if (pull.norm() > 1e-4f) {
                float stiffness = 30.0f;
                Eigen::Vector2f force = pull * stiffness * timeStep;
                poly->applyImpulseAt(grabStart, force);
            }
        }
    }

    // UI rendering
    setScreenSpaceProjection(window);
    glLoadIdentity();  // Reset modelview again

    for (const auto& button : buttons) {
        button.draw(button.getTool() == currentTool);
    }

}

void resetScene(GLFWwindow* window) {
    polygons.clear();
    selectedPolygons.clear();
    cameraPosition = Eigen::Vector2f(0.0f, 0.0f);
    cameraZoom = 1.0f;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    updateProjection(window);

    LoadScene(1);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (key == GLFW_KEY_R) {
            resetScene(window);
        }

        switch (key) {
        case GLFW_KEY_1:
            switchTool(Tool::Eraser);
            break;
        case GLFW_KEY_2:
            switchTool(Tool::Pencil);
            break;
        case GLFW_KEY_3:
            switchTool(Tool::Flick);
            break;
        case GLFW_KEY_4:
            switchTool(Tool::Grab);
            break;
        case GLFW_KEY_5:
            switchTool(Tool::Select);
            break;
        case GLFW_KEY_6:
            switchTool(Tool::View);
            break;
        }

        if (key == GLFW_KEY_SPACE && !isQuickSwapping) {
            isQuickSwapping = true;
            previousTool = currentTool;
            switchTool(Tool::View);
        }
    }

    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_SPACE && isQuickSwapping) {
            isQuickSwapping = false;
            switchTool(previousTool);
        }
    }
}


void handlePencilToolRepeat(GLFWwindow* window) {
    if (currentTool != Tool::Pencil || uiHovered) return;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - lastPencilTime >= toolRepeatDelay) {
            polygons.push_back(
                PolygonFactory::CreateRegularPolygon(
                    Vector3d(pencilMousePos.x(), pencilMousePos.y(), 0.0),
                    pencilSides,
                    pencilSizeX, pencilSizeY,
                    pencilRotation
                )
            );
            lastPencilTime = now;
        }
    }
}

void eraserUpdate(GLFWwindow* window) {
    if (currentTool != Tool::Eraser || uiHovered) return;
    double sx, sy;
    glfwGetCursorPos(window, &sx, &sy);
    Eigen::Vector2f worldClick = screenToWorld(window, sx, sy);
    updateEraserHoverOutlines(worldClick);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) return;

    std::shared_ptr<Polygon> clickedPolygon = nullptr;

    for (auto& poly : polygons) {
        if (poly->containsPoint(worldClick, 0.05f)) {
            clickedPolygon = poly;
            break;
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (clickedPolygon) {
            // Track countdown
            if (eraserCountdowns.find(clickedPolygon) == eraserCountdowns.end()) {
                eraserCountdowns[clickedPolygon] = 1;
            }
            else {
                eraserCountdowns[clickedPolygon]++;
            }

            if (eraserCountdowns[clickedPolygon] >= eraserDelayFrames) {
                bool isSelected = std::find(selectedPolygons.begin(), selectedPolygons.end(), clickedPolygon) != selectedPolygons.end();

                if (isSelected) {
                    for (const auto& poly : selectedPolygons) {
                        polygons.erase(std::remove(polygons.begin(), polygons.end(), poly), polygons.end());
                    }
                    selectedPolygons.clear();
                }
                else {
                    polygons.erase(std::remove(polygons.begin(), polygons.end(), clickedPolygon), polygons.end());
                    clearSelection();
                }

                eraserCountdowns.clear(); // Reset all after a deletion
            }
        }
        else {
            eraserCountdowns.clear(); // Reset if not hovering any polygon
        }
    }
    else {
        eraserCountdowns.clear(); // Reset if mouse is not pressed
    }

}

void updateUIHover(GLFWwindow* window) {
    double sx, sy;
    glfwGetCursorPos(window, &sx, &sy);
    float mouseX = static_cast<float>(sx);
    float mouseY = static_cast<float>(sy);

    for (const auto& b : buttons) {
        if (b.isHovered(mouseX, mouseY)) {
            uiHovered = true;
            return;
        }
    }

    uiHovered = false;
}


int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, "Polygon Playground", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    glewInit();

    // For alpha of images
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    arrowCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    crosshairCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    ibeamCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

    // Set up initial viewport and projection
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    initScenes();
    initButtons();

    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    while (!glfwWindowShouldClose(window)) {
        updateUIHover(window);
        handlePencilToolRepeat(window);
        eraserUpdate(window);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

