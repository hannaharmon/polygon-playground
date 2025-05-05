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

#include "SceneManager.h"
#include "PolygonFactory.h"
#include "Polygon.h"
#include "Particle.h"

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

std::shared_ptr<Polygon> selectedPolygon = nullptr;

bool flickActive = false;
Eigen::Vector2f flickStartCenterOffset;
Eigen::Vector2f flickCurrent;

bool grabActive = false;
Eigen::Vector2f grabStartCenterOffset;
Eigen::Vector2f grabCurrent;

enum class Tool { None, View, Flick, Grab, Select, Pencil, Eraser };
Tool currentTool = Tool::Flick;

GLFWcursor* arrowCursor;
GLFWcursor* handCursor;
GLFWcursor* crosshairCursor;
GLFWcursor* ibeamCursor;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    float aspect = width / (float)height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspect >= 1.0f) {
        glOrtho(-2.0f * aspect, 2.0f * aspect, -2.0f, 2.0f, -1.0f, 1.0f);
    }
    else {
        glOrtho(-2.0f, 2.0f, -2.0f / aspect, 2.0f / aspect, -1.0f, 1.0f);
    }

    glMatrixMode(GL_MODELVIEW);
}

Eigen::Vector2f screenToWorld(GLFWwindow* window, double sx, double sy) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    float aspect = width / (float)height;

    float ndcX = (float)(2.0 * sx / width - 1.0);
    float ndcY = (float)(1.0 - 2.0 * sy / height); // y is flipped in OpenGL

    float worldX, worldY;

    if (aspect >= 1.0f) {
        worldX = ndcX * 2.0f * aspect;
        worldY = ndcY * 2.0f;
    }
    else {
        worldX = ndcX * 2.0f;
        worldY = ndcY * 2.0f / aspect;
    }

    return Eigen::Vector2f(worldX, worldY);
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    switch (codepoint) {
    case 's': 
        currentTool = Tool::Select; 
        glfwSetCursor(window, arrowCursor);
        break;
    case 'f': 
        currentTool = Tool::Flick; 
        glfwSetCursor(window, crosshairCursor);
        break;
    case 'g': 
        currentTool = Tool::Grab; 
        glfwSetCursor(window, handCursor);
        break;
    case 'p': 
        currentTool = Tool::Pencil;
        glfwSetCursor(window, ibeamCursor);
        break;
    case 'e': 
        currentTool = Tool::Eraser;
        glfwSetCursor(window, arrowCursor);
        break;
    case 'v': 
        currentTool = Tool::View; 
        glfwSetCursor(window, arrowCursor);
        break;
    default: break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double sx, sy;
    glfwGetCursorPos(window, &sx, &sy);
    Eigen::Vector2f worldClick = screenToWorld(window, sx, sy);

    switch (currentTool) {
    case Tool::Flick:
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                for (auto& poly : polygons) {
                    if (poly->containsPoint(worldClick, 0.05f)) {
                        selectedPolygon = poly;
                        flickActive = true;
                        poly->outlineColor = Eigen::Vector3f(1.0f, 1.0f, 0.0f);
                        flickStartCenterOffset = worldClick - poly->getCenter();
                        flickCurrent = worldClick;
                        break;
                    }
                }
            }
            else if (action == GLFW_RELEASE && selectedPolygon) {
                flickActive = false;
                Eigen::Vector2f center = selectedPolygon->getCenter();
                Eigen::Vector2f flickStartWorld = center + flickStartCenterOffset;
                Eigen::Vector2f flickDir = flickStartWorld - flickCurrent;
                if (flickDir.norm() > 1e-4) {
                    selectedPolygon->applyImpulseAt(flickStartWorld, flickDir * flickForceScale);
                }
                selectedPolygon->outlineColor = selectedPolygon->defaultOutlineColor;
                selectedPolygon = nullptr;
            }
        }
        break;

    case Tool::Grab:
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                for (auto& poly : polygons) {
                    if (poly->containsPoint(worldClick, 0.05f)) {
                        selectedPolygon = poly;
                        grabActive = true;
                        poly->outlineColor = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
                        grabStartCenterOffset = worldClick - poly->getCenter();
                        grabCurrent = worldClick;
                        break;
                    }
                }
            }
            else if (action == GLFW_RELEASE && selectedPolygon) {
                grabActive = false;
                selectedPolygon->outlineColor = selectedPolygon->defaultOutlineColor;
                selectedPolygon = nullptr;
            }
        }
        break;

    default:
        break;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Eigen::Vector2f world = screenToWorld(window, xpos, ypos);
    if (flickActive) {
        flickCurrent = world;
    }
    if (grabActive) {
        grabCurrent = world;
    }
}

void LoadScene(int key) {
	sceneManager.LoadScene(key);
	polygons = sceneManager.GetPolygons();
}

void initScenes() {
    sceneManager.RegisterScene(1, []() {
        return PolygonFactory::CreateStackedRectangles(Vector3d(0, -.8, 0), 4, 0.2, 0.4);
        });

    sceneManager.RegisterScene(2, []() {
        return PolygonFactory::CreateWall(Vector3d(-1.2, -.8, 0), 3, 3, 0.4, 0.4);
        });

    sceneManager.RegisterScene(3, []() {
        return PolygonFactory::CreateGridOfPolygons(Vector3d(0, 0.5, 0), 2, 3, 6, 0.5, 0.5, 0.1, 0.1);
        });

    // Load the first scene by default
	LoadScene(1);
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

    glClear(GL_COLOR_BUFFER_BIT);
    for (auto& poly : polygons) {
        poly->draw();
    }

    if (flickActive) {
        glLineWidth(3.0f);
        glColor3f(1.0f, 1.0f, 0.0f);

        glBegin(GL_LINES);
        Eigen::Vector2f start = selectedPolygon->getCenter() + flickStartCenterOffset;
        glVertex2f(start.x(), start.y());
        glVertex2f(flickCurrent.x(), flickCurrent.y());
        glEnd();
    }

    else if (grabActive) {
        glLineWidth(3.0f);
        glColor3f(0.0f, 1.0f, 0.0f);

        glBegin(GL_LINES);
        Eigen::Vector2f start = selectedPolygon->getCenter() + grabStartCenterOffset;
        glVertex2f(start.x(), start.y());
        glVertex2f(grabCurrent.x(), grabCurrent.y());
        glEnd();

        // Apply grab force
        if (grabActive && selectedPolygon) {
            Eigen::Vector2f grabWorldStart = selectedPolygon->getCenter() + grabStartCenterOffset;
            Eigen::Vector2f pullVec = grabCurrent - grabWorldStart;

            if (pullVec.norm() > 1e-4f) {
                float stiffness = 30.0f; // can tweak
                Eigen::Vector2f force = pullVec * stiffness * timeStep;

                selectedPolygon->applyImpulseAt(grabWorldStart, force);
            }
        }
    }

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            int sceneIndex = key - GLFW_KEY_0;
			LoadScene(sceneIndex);
        }
    }
}

int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, "Polygon Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    glewInit();

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

    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

