#include <iostream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <Eigen/Dense>

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
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
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

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Polygon Playground", NULL, NULL);

    if (!window) {
        cerr << "Failed to open GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    glewInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2, 2, -2, 2, -1, 1); // keep logical coordinates fixed
    glMatrixMode(GL_MODELVIEW);

    initScenes();

    glfwSetKeyCallback(window, KeyCallback);

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

