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

#include "Polygon.h"

using namespace std;
using namespace Eigen;

// Simulation parameters
const double timeStep = 1.0 / 60.0;
const Vector3d gravity(0.0, -9.8, 0.0);
const double groundY = -1.0;
const double damping = 0.98;

// Globals
vector<shared_ptr<Polygon>> polygons;

void initBlocks() {
    // Upright square
    polygons.push_back(make_shared<Polygon>(
        Vector3d(0, 1.0, 0),
        4,                  // numEdges
        0.8, 0.4,           // width, height
        M_PI / 4            // rotate it into upright position
    ));
    polygons.push_back(make_shared<Polygon>(
        Vector3d(0, 2.0, 0),
        4,                  // numEdges
        0.8, 0.4,           // width, height
        M_PI / 4            // rotate it into upright position
    ));
}

void display(GLFWwindow* window) {
    for (auto& poly : polygons) {
        poly->step(
            timeStep,
            5,                // spring iterations
            10,               // collision iterations
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

int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Polygon Playground", NULL, NULL);
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
    glOrtho(-2, 2, -2, 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    initBlocks();

    while (!glfwWindowShouldClose(window)) {
        display(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
