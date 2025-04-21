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
shared_ptr<Polygon> polygon;

void initBlocks() {
	polygon = make_shared<Polygon>(Vector3d(0, 0.5, 0.0), 4, 0.4, 0.4);
}

void display(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT);

    
}

int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "2D Deformable Block", NULL, NULL);
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
        polygon->applyForces(timeStep, gravity, damping);
        polygon->satisfyConstraints(10, groundY);
        polygon->updateVelocities(timeStep);

        glClear(GL_COLOR_BUFFER_BIT);
        polygon->draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
