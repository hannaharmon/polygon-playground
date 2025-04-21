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

Polygon::Polygon(const Vector3d& pos, int numEdges, double width, double height, double rotation) {
    generateRegularPolygon(pos, numEdges, width, height, rotation);
}

void Polygon::generateRegularPolygon(const Vector3d& center, int numEdges, double width, double height, double rotation) {
    double radiusX = width / 2.0;
    double radiusY = height / 2.0;

    // Create one particle per corner
    for (int i = 0; i < numEdges; ++i) {
        double angle = 2.0 * M_PI * i / numEdges + rotation;
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

    // Min edge length
    double minLength = std::numeric_limits<double>::max();
    for (const auto& edge : edges) {
        double len = (edge.p0->x - edge.p1->x).norm();
        if (len < minLength) minLength = len;
    }
    collisionThickness = 0.5 * minLength;
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

void Polygon::resolveCollisionsWith(const std::shared_ptr<Polygon>& other, double timeStep) {
    for (const auto& edge : edges) {
        Vector3d a = edge.p0->x;
        Vector3d b = edge.p1->x;
        Vector3d edgeVec = b - a;
        double edgeLenSq = edgeVec.squaredNorm();

        for (auto& p : other->particles) {
            if (p->fixed) continue;

            Vector3d ap = p->x - a;
            double t = edgeVec.dot(ap) / edgeLenSq;
            t = std::clamp(t, 0.0, 1.0);
            Vector3d closest = a + t * edgeVec;

            Vector3d delta = p->x - closest;
            double dist = delta.norm();
            double minDist = std::max(collisionThickness, other->collisionThickness);

            if (dist < minDist && dist > 1e-6) {
                Vector3d n = delta.normalized();
                double penetration = minDist - dist;

                // Inverse masses
                double wp = p->fixed ? 0.0 : 1.0 / p->m;
                double w0 = edge.p0->fixed ? 0.0 : 1.0 / edge.p0->m;
                double w1 = edge.p1->fixed ? 0.0 : 1.0 / edge.p1->m;
                double wsum = wp + w0 + w1;
                if (wsum < 1e-8) continue;

                // --- Position Correction (normal) ---
                if (!p->fixed)
                    p->x += n * (penetration * wp / wsum);
                if (!edge.p0->fixed)
                    edge.p0->x -= n * (penetration * w0 / wsum);
                if (!edge.p1->fixed)
                    edge.p1->x -= n * (penetration * w1 / wsum);

                // --- Normal velocity damping ---
                double vRelNormal = p->v.dot(n);
                if (vRelNormal < 0.0) {
                    p->v -= vRelNormal * n;
                }

                // --- Tangential Friction ---
                Vector3d edgeVel = 0.5 * (edge.p0->v + edge.p1->v);
                Vector3d relVel = p->v - edgeVel;

                Vector3d vnVec = relVel.dot(n) * n;
                Vector3d vt = relVel - vnVec;

                if (vt.norm() > 1e-6) {
                    double mu = 0.6;

                    // Use dynamic friction if significant normal contact
                    double frictionMag = mu * std::abs(vRelNormal);

                    // Fallback if vRelNormal is tiny (almost at rest) — simulate static friction
                    if (std::abs(vRelNormal) < 1e-4) {
                        frictionMag = mu * penetration / timeStep;
                    }

                    frictionMag = std::min(frictionMag, vt.norm());

                    Vector3d frictionImpulse = -frictionMag * vt.normalized();

                    if (!p->fixed)
                        p->v += frictionImpulse * (wp / wsum);
                    if (!edge.p0->fixed)
                        edge.p0->v -= frictionImpulse * (w0 / wsum);
                    if (!edge.p1->fixed)
                        edge.p1->v -= frictionImpulse * (w1 / wsum);
                }

            }
        }
    }
}

double Polygon::getTotalMass() const {
    double mass = 0.0;
    for (const auto& p : particles)
        mass += p->m;
    return mass;
}


void Polygon::updateVelocities(double timeStep) {
    for (auto& p : particles) {
        if (!p->fixed) {
            p->v = (p->x - p->p) / timeStep;
        }
        if (!p->fixed && std::abs(p->v.x()) < 0.02 && std::abs(p->v.y()) < 0.01) {
            p->v.x() = 0;
        }
    }
}

void Polygon::applyGroundFriction(double groundY, const Vector3d& gravity, double timeStep) {
    double mu = 0.8;

    // Compute total downward force on the polygon
    double totalMass = getTotalMass();
    double normalForce = totalMass * gravity.norm();
    double maxFriction = mu * normalForce * timeStep;

    // Identify how many particles are in contact with the ground
    std::vector<std::shared_ptr<Particle>> groundParticles;
    for (auto& p : particles) {
        if (!p->fixed && std::abs(p->x.y() - groundY) < 1e-4) {
            groundParticles.push_back(p);
        }
    }

    if (groundParticles.empty()) return;

    // Distribute max friction across grounded particles
    double frictionPerParticle = maxFriction / groundParticles.size();

    for (auto& p : groundParticles) {
        double vx = p->v.x();
        if (std::abs(vx) > 1e-4) {
            double friction = std::clamp(-vx, -frictionPerParticle, frictionPerParticle);
            p->v.x() += friction;
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
                resolveCollisionsWith(other, timeStep);
            }
        }
    }

    // 3. Satisfy spring constraints
    for (int k = 0; k < springIters; ++k) {
        for (auto& s : springs) {
            Vector3d delta = s->p1->x - s->p0->x;
            double dist = delta.norm();

            // Prevent divide by zero
            if (dist < 1e-6) continue;

            // Relative correction
            double diff = (dist - s->L) / dist;

            // Apply spring correction
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

    // 6. Ground friction
    applyGroundFriction(groundY, gravity, timeStep);
}

void drawPolygonOffset(
    const std::vector<std::shared_ptr<Particle>>& particles,
    float offset,
    bool fill,
    const Eigen::Vector3f& color,
    float lineWidth = 2.5f
) {
    using Vec2 = Eigen::Vector2f;

    // Compute center of shape
    Vec2 center(0, 0);
    for (auto& p : particles) {
        center += Vec2(p->x.x(), p->x.y());
    }
    center /= particles.size();

    // Set color
    glColor3f(color.x(), color.y(), color.z());

    // Choose draw mode
    if (fill) {
        glBegin(GL_TRIANGLE_FAN);
    }
    else {
        glLineWidth(lineWidth);
        glBegin(GL_LINE_LOOP);
    }

    // Shift and draw
    for (auto& p : particles) {
        Vec2 pos(p->x.x(), p->x.y());
        Vec2 dir = (pos - center).normalized();
        Vec2 shifted = pos + dir * offset;
        glVertex2f(shifted.x(), shifted.y());
    }

    glEnd();
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
        glLineWidth(1);
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
        glLineWidth(1);
        glBegin(GL_LINES);
        glColor3f(0.0f, 0.5f, 1.0f);
        for (auto& e : edges) {
            glVertex2f(e.p0->x.x(), e.p0->x.y());
            glVertex2f(e.p1->x.x(), e.p1->x.y());
        }
        glEnd();
    }

    drawPolygonOffset(particles, .04f, true, Eigen::Vector3f(0.0f, 0.5f, 1.0f));
    drawPolygonOffset(particles, .04, false, Eigen::Vector3f(1, 1, 1));
}
