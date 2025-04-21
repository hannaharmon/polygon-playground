#include "Polygon.h"
#include "Particle.h"
#include "Spring.h"

#include <GL/glew.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace Eigen;

Polygon::Polygon(const Vector3d& pos, int numEdges, double width, double height) {
    generateRegularPolygon(pos, numEdges, width, height);
}

void Polygon::generateRegularPolygon(const Vector3d& center, int numEdges, double width, double height) {
    double radiusX = width / 2.0;
    double radiusY = height / 2.0;

    // Create one particle per corner
    for (int i = 0; i < numEdges; ++i) {
        double angle = 2.0 * M_PI * i / numEdges;
        auto p = make_shared<Particle>();
        p->x = center + Vector3d(radiusX * cos(angle), radiusY * sin(angle), 0);
        p->v = Vector3d::Zero();
        p->fixed = false;
        particles.push_back(p);
    }

    for (int i = 0; i < numEdges; ++i) {
        int next = (i + 1) % numEdges;
        int prev = (i - 1 + numEdges) % numEdges;

        // Structural spring (edge)
        springs.push_back(make_shared<Spring>(particles[i], particles[next], 1.0));
        edges.push_back({ particles[i], particles[next] });

        // Shear springs (for quadrilaterals and up)
        if (numEdges >= 4) {
            springs.push_back(make_shared<Spring>(particles[i], particles[prev], 1.0));
        }

        // Bending springs (connect to next-next)
        if (numEdges >= 4) {
            int next2 = (i + 2) % numEdges;
            springs.push_back(make_shared<Spring>(particles[i], particles[next2], 1.0));
        }
    }

    // Compute average edge length to set collision thickness
    double totalLength = 0.0;
    for (const auto& s : springs) {
        totalLength += (s->p1->x - s->p0->x).norm();
    }
    double avgLength = totalLength / springs.size();
    collisionThickness = 0.25 * avgLength + .03;

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

void Polygon::resolveCollisionsWith(const shared_ptr<Polygon>& other) {
    for (const auto& edge : edges) {
        Vector3d a = edge.p0->x;
        Vector3d b = edge.p1->x;
        Vector3d edgeVec = b - a;
        double edgeLenSq = edgeVec.squaredNorm();

        for (auto& p : other->particles) {
            if (p->fixed) continue;

            Vector3d ap = p->x - a;
            double t = edgeVec.dot(ap) / edgeLenSq;
            t = std::max(0.0, std::min(1.0, t));
            Vector3d closest = a + t * edgeVec;
            Vector3d delta = p->x - closest;
            double dist = delta.norm();
            double minDist = std::max(collisionThickness, other->collisionThickness);

            if (dist < minDist && dist > 1e-6) {
                Vector3d n = delta.normalized();
                double penetration = minDist - dist;

                double w0 = edge.p0->fixed ? 0.0 : 1.0 / edge.p0->m;
                double w1 = edge.p1->fixed ? 0.0 : 1.0 / edge.p1->m;
                double wp = p->fixed ? 0.0 : 1.0 / p->m;
                double wsum = w0 + w1 + wp;
                if (wsum < 1e-6) continue;

                if (!p->fixed)
                    p->x += n * (penetration * wp / wsum);
                if (!edge.p0->fixed)
                    edge.p0->x -= n * (penetration * w0 / wsum);
                if (!edge.p1->fixed)
                    edge.p1->x -= n * (penetration * w1 / wsum);
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

void Polygon::step(
    double timeStep,
    int springIters,
    int collisionIters,
    double groundY,
    const std::vector<std::shared_ptr<Polygon>>& others,
    const Vector3d& gravity,
    double damping)
{
    // 1. Apply forces
    applyForces(timeStep, gravity, damping);

    // 2. Resolve overlaps (edge-based)
    for (int k = 0; k < collisionIters; ++k) {
        for (auto& other : others) {
            if (other.get() != this) {
                resolveCollisionsWith(other);
            }
        }
    }

    // 3. Satisfy spring constraints
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

    // 4. Ground collision
    for (auto& p : particles) {
        if (!p->fixed && p->x.y() < groundY) {
            p->x.y() = groundY;
            if (p->v.y() < 0.0) p->v.y() = 0.0;
        }
    }

    // 5. Update velocity
    updateVelocities(timeStep);
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
