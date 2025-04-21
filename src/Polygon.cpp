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

	// Determine edges
    // Top and bottom rows
    for (int j = 0; j < cols - 1; ++j) {
        // Top row
        edges.push_back({ particles[j], particles[j + 1] });
        // Bottom row
        int rowOffset = (rows - 1) * cols;
        edges.push_back({ particles[rowOffset + j], particles[rowOffset + j + 1] });
    }

    // Left and right columns
    for (int i = 0; i < rows - 1; ++i) {
        // Left column
        edges.push_back({ particles[i * cols], particles[(i + 1) * cols] });
        // Right column
        edges.push_back({ particles[i * cols + (cols - 1)], particles[(i + 1) * cols + (cols - 1)] });
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

void Polygon::satisfyConstraints(int springIters, int collisionIters, double groundY, const std::vector<std::shared_ptr<Polygon>>& others)
{
    // Spring Constraints
    for (int k = 0; k < springIters; ++k) {
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

    // Collision correction
    for (int k = 0; k < collisionIters; ++k) {
        for (auto& other : others) {
            if (other.get() == this) continue;
            this->resolveCollisionsWith(other);
        }

        // Ground
        for (auto& p : particles) {
            if (!p->fixed && p->x.y() < groundY) {
                p->x.y() = groundY;
                if (p->v.y() < 0.0) p->v.y() = 0.0;
                if (p->v.norm() < 0.05) p->v = Vector3d::Zero();
            }
        }
    }
}

void Polygon::resolveCollisionsWith(const std::shared_ptr<Polygon>& other) {

	// edge based collision detection
    for (const auto& edge : edges) {
        Vector3d a = edge.p0->x;
        Vector3d b = edge.p1->x;
        Vector3d edgeVec = b - a;
        double edgeLenSq = edgeVec.squaredNorm();

        for (auto& p : other->particles) {
            if (p->fixed) continue;

            // Project point onto edge segment
            Vector3d ap = p->x - a;
            double t = edgeVec.dot(ap) / edgeLenSq;
            t = std::max(0.0, std::min(1.0, t));
            Vector3d closest = a + t * edgeVec;

            Vector3d delta = p->x - closest;
            double dist = delta.norm();
            double minDist = std::min(collisionThickness, other->collisionThickness);

            if (dist < minDist && dist > 1e-6) {
                Vector3d n = delta.normalized();
                double penetration = minDist - dist;

                // Distribute correction across the three points
                double w0 = edge.p0->fixed ? 0.0 : 1.0 / edge.p0->m;
                double w1 = edge.p1->fixed ? 0.0 : 1.0 / edge.p1->m;
                double wp = p->fixed ? 0.0 : 1.0 / p->m;
                double wsum = w0 + w1 + wp;

                if (wsum < 1e-8) continue; // all are fixed

                if (!p->fixed)
                    p->x += n * (penetration * wp / wsum);
                if (!edge.p0->fixed)
                    edge.p0->x -= n * (penetration * w0 / wsum);
                if (!edge.p1->fixed)
                    edge.p1->x -= n * (penetration * w1 / wsum);

                // Damp velocity on p only (for simplicity)
                double vRel = p->v.dot(n);
                if (vRel < 0.0)
                    p->v -= vRel * n;
            }
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

void Polygon::draw(bool drawParticles, bool drawSprings, bool drawEdges) const {

    // particles
    if (drawParticles) {
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glColor3f(1, 0, 0);
        for (auto& p : particles) {
            glVertex2f((float)p->x.x(), (float)p->x.y());
        }
        glEnd();
    }

	// springs
    if (drawSprings) {
        glBegin(GL_LINES);
        glColor3f(0, 1, 0);
        for (auto& s : springs) {
            glVertex2f((float)s->p0->x.x(), (float)s->p0->x.y());
            glVertex2f((float)s->p1->x.x(), (float)s->p1->x.y());
        }
        glEnd();
    }

    // edges
    if (drawEdges) {
        glBegin(GL_LINES);
        glColor3f(0.0f, 0.5f, 1.0f);
        for (auto& e : edges) {
            glVertex2f(e.p0->x.x(), e.p0->x.y());
            glVertex2f(e.p1->x.x(), e.p1->x.y());
        }
        glEnd();
    }
}
