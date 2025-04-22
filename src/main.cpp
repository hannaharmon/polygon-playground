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

using namespace std;
using namespace Eigen;

// Simulation parameters
const double timeStep = 1.0 / 60.0;
const Vector3d gravity(0.0, -9.8, 0.0);
const double groundY = -1.0;
const double damping = 0.98;

// Globals
SceneManager sceneManager;
vector<shared_ptr<Polygon>> polygons;
GLFWwindow* window;

shared_ptr<Polygon> selectedPolygon = nullptr;

bool flickActive = false;
Eigen::Vector2f flickStart;
Eigen::Vector2f flickCurrent;

bool grabActive = false;
Eigen::Vector2f grabStart;
Eigen::Vector2f grabCurrent;


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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Flick
    if (button == GLFW_MOUSE_BUTTON_RIGHT && !grabActive) {
        if (action == GLFW_PRESS) {
            double sx, sy;
            glfwGetCursorPos(window, &sx, &sy);
            Eigen::Vector2f worldClick = screenToWorld(window, sx, sy);

            for (auto& poly : polygons) {
                if (poly->containsPoint(worldClick, 0.05f)) { // Use same offset as drawPolygonOffset
                    flickStart = worldClick;
                    flickCurrent = worldClick;
                    flickActive = true;
					selectedPolygon = poly;
                    poly->outlineColor = Eigen::Vector3f(1.0f, 1.0f, 0.0f);
                    break;
                }
            }
        }

        else if (action == GLFW_RELEASE) {
            flickActive = false;
            if (selectedPolygon) {
                // Compute the flick vector (from current to start — opposite direction)
                Eigen::Vector2f delta = flickStart - flickCurrent;

                // Scale the impulse (tweak the scale factor as needed)
                float impulseStrength = 5.0f; // You can tune this
                Eigen::Vector2f impulse = delta * impulseStrength;

                selectedPolygon->applyImpulseAt(flickStart, impulse);

                selectedPolygon->outlineColor = selectedPolygon->defaultOutlineColor;
                selectedPolygon = nullptr;
            }
        }

    }

    // Grab
    else if (button == GLFW_MOUSE_BUTTON_LEFT && !flickActive) {
        if (action == GLFW_PRESS) {
            double sx, sy;
            glfwGetCursorPos(window, &sx, &sy);
            Eigen::Vector2f worldClick = screenToWorld(window, sx, sy);

            for (auto& poly : polygons) {
                if (poly->containsPoint(worldClick, 0.05f)) { // Use same offset as drawPolygonOffset
                    grabStart = worldClick;
                    grabCurrent = worldClick;
                    grabActive = true;
					selectedPolygon = poly;
                    poly->outlineColor = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
                    break;
                }
            }
        }

        else if (action == GLFW_RELEASE) {
            grabActive = false;
            if (selectedPolygon) {
                selectedPolygon->outlineColor = selectedPolygon->defaultOutlineColor;
                selectedPolygon = nullptr;
            }
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (flickActive) {
        flickCurrent = screenToWorld(window, xpos, ypos);
    }
    else if (grabActive) {
        grabCurrent = screenToWorld(window, xpos, ypos);
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
            5,                // spring iterations
            12,               // collision iterations
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
        glVertex2f(flickStart.x(), flickStart.y());
        glVertex2f(flickCurrent.x(), flickCurrent.y());
        glEnd();
    }

    else if (grabActive) {
        glLineWidth(3.0f);
        glColor3f(0.0f, 1.0f, 0.0f);

        glBegin(GL_LINES);
        glVertex2f(grabStart.x(), grabStart.y());
        glVertex2f(grabCurrent.x(), grabCurrent.y());
        glEnd();
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

    // Set up initial viewport and projection
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    initScenes();

    glfwSetKeyCallback(window, key_callback);
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

