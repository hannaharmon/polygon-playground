#include <iostream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <Eigen/Dense>

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
    int edges = 4;     // Keeping rectangular for now
    double width = 0.4;
    double height = 0.4;

    polygons.push_back(make_shared<Polygon>(Vector3d(0.0, .2, 0.0), edges, width, height));
    polygons.push_back(make_shared<Polygon>(Vector3d(0.0, 2, 0.0), edges, width, height));
    polygons.push_back(make_shared<Polygon>(Vector3d(0.0, 3, 0.0), edges, width, height));
    polygons.push_back(make_shared<Polygon>(Vector3d(1, 2, 0.0), edges, width, height));
    polygons.push_back(make_shared<Polygon>(Vector3d(1, .8, 0.0), edges, width, height));
}

void display(GLFWwindow* window) {
    for (auto& poly : polygons) {
        poly->applyForces(timeStep, gravity, damping);
    }

    int springIters = 5;
    int collisionIters = 2;
    for (size_t i = 0; i < polygons.size(); ++i) {
        for (size_t j = 0; j < polygons.size(); ++j) {
            if (i == j) continue;
            polygons[i]->resolveCollisionsWith(polygons[j]);
        }
    }
    for (auto& poly : polygons) {
        poly->satisfyConstraints(springIters, collisionIters, groundY, polygons);
    }

    for (auto& poly : polygons) {
        poly->updateVelocities(timeStep);
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
