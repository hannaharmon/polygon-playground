#include "Polygon.h"
#include "Particle.h"
#include "Spring.h"

#include <GL/glew.h>

using namespace std;
using namespace Eigen;

Polygon::Polygon(const Vector3d& pos, int numEdges, double width, double height) {
	// For now, only support "box" shapes
    if (numEdges == 4) {
        generateRectangularGrid(pos, 2, 2, width, height);
    }
    else {
        // TODO: implement polygonal generation
    }
}

void Polygon::generateRectangularGrid(const Vector3d& pos, int rows, int cols, double width, double height) {
    double dx = width / (cols - 1);
    double dy = height / (rows - 1);

    // Create particles
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            auto p = make_shared<Particle>();
            p->x = pos + Vector3d(j * dx - width / 2, -i * dy + height / 2, 0);
            p->v = Vector3d::Zero();
            p->fixed = false;
            particles.push_back(p);
        }
    }

    // Create springs
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int k = i * cols + j;

            // Structural
            if (j < cols - 1) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + 1], 1.0)); // right
            }
            if (i < rows - 1) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + cols], 1.0)); // down
            }

            // Shear
            if (i < rows - 1 && j < cols - 1) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + cols + 1], 1.0)); // down-right
            }
            if (i < rows - 1 && j > 0) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + cols - 1], 1.0)); // down-left
            }

            // Bending
            if (j < cols - 2) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + 2], 1.0)); // right-2
            }
            if (i < rows - 2) {
                springs.push_back(make_shared<Spring>(particles[k], particles[k + 2 * cols], 1.0)); // down-2
            }
        }
    }
}

void Polygon::applyForces(double timeStep, const Vector3d& gravity, double damping) {
    for (auto& p : particles) {
        if (!p->fixed) {
            p->p = p->x;
            p->v += gravity * timeStep;
            p->v *= damping;
            p->x += p->v * timeStep;
        }
    }
}

void Polygon::satisfyConstraints(int iterations, double groundY) {
    for (int k = 0; k < iterations; ++k) {
        for (auto& s : springs) {
            Vector3d delta = s->p1->x - s->p0->x;
            double dist = delta.norm();
            if (dist < 1e-6) continue;
            double diff = (dist - s->L) / dist;
            if (!s->p0->fixed && !s->p1->fixed) {
                Vector3d correction = 0.5 * diff * delta;
                s->p0->x += correction;
                s->p1->x -= correction;
            }
            else if (!s->p0->fixed) {
                s->p0->x += diff * delta;
            }
            else if (!s->p1->fixed) {
                s->p1->x -= diff * delta;
            }
        }
    }

    // Ground collision
    for (auto& p : particles) {
        if (!p->fixed && p->x.y() < groundY) {
            p->x.y() = groundY;
            if (p->v.y() < 0.0) p->v.y() = 0.0;
            if (p->v.norm() < 0.05) p->v = Vector3d::Zero();
        }
    }
}

void Polygon::updateVelocities(double timeStep) {
    for (auto& p : particles) {
        if (!p->fixed) {
            p->v = (p->x - p->p) / timeStep;
        }
    }
}

void Polygon::draw() const {
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glColor3f(1, 0, 0);
    for (auto& p : particles) {
        glVertex2f((float)p->x.x(), (float)p->x.y());
    }
    glEnd();

    glBegin(GL_LINES);
    glColor3f(0, 1, 0);
    for (auto& s : springs) {
        glVertex2f((float)s->p0->x.x(), (float)s->p0->x.y());
        glVertex2f((float)s->p1->x.x(), (float)s->p1->x.y());
    }
    glEnd();
}
