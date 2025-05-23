#include "Polygon.h"
#include "Particle.h"
#include "Spring.h"

#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <algorithm>  // for std::find

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace Eigen;

Polygon::Polygon(const Vector3d& pos, int numEdges, double width, double height, double rotation) {
    generateRegularPolygon(pos, numEdges, width, height, rotation);
}


Polygon::Polygon(const Polygon& other)
    : fillColor(other.fillColor),
    outlineColor(other.outlineColor),
    defaultOutlineColor(other.defaultOutlineColor)
{
    // Step 1: Deep-copy particles
    particles.reserve(other.particles.size());
    for (const auto& p : other.particles) {
        particles.push_back(std::make_shared<Particle>(*p));  // clone the particle
    }

    // Step 2: Deep-copy springs
    springs.reserve(other.springs.size());
    for (const auto& s : other.springs) {
        // Find indices of spring endpoints in original polygon
        auto it0 = std::find(other.particles.begin(), other.particles.end(), s->p0);
        auto it1 = std::find(other.particles.begin(), other.particles.end(), s->p1);

        assert(it0 != other.particles.end() && it1 != other.particles.end());
        int i0 = static_cast<int>(std::distance(other.particles.begin(), it0));
        int i1 = static_cast<int>(std::distance(other.particles.begin(), it1));

        springs.push_back(std::make_shared<Spring>(particles[i0], particles[i1], s->alpha));
    }
}


Eigen::Vector2f Polygon::getCenter() const {
    Eigen::Vector2f center(0, 0);
    for (const auto& p : particles) {
        center += Eigen::Vector2f(p->x.x(), p->x.y());
    }
    center /= (float)particles.size();
    return center;
}

void Polygon::moveCenterTo(const Eigen::Vector3d& target) {
    Eigen::Vector2f target2f = target.head<2>().cast<float>();
    Eigen::Vector2f currentCenter = getCenter();
    Eigen::Vector2f offset = target2f - currentCenter;

    for (auto& p : particles) {
        p->x.head<2>() += offset.cast<double>();
        p->p.head<2>() += offset.cast<double>();
    }
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

    double totalLength = 0.0;
    for (const auto& edge : edges) {
        totalLength += (edge.p0->x - edge.p1->x).norm();
    }
    double avgLength = totalLength / edges.size();
    collisionThickness = .1; // More conservative, consistent
}


void Polygon::applyForces(double timeStep, const Vector3d& gravity, double damping) {
    for (auto& p : particles) {
        if (!p->fixed) {
            p->p = p->x;
            p->v += gravity * timeStep;
            p->v *= damping;
        }
    }
}

bool Polygon::isTouching(const std::shared_ptr<Polygon>& other) const {
    auto getEdges = [](const std::vector<std::shared_ptr<Particle>>& particles) {
        std::vector<std::pair<Vector2d, Vector2d>> edges;
        int n = particles.size();
        for (int i = 0; i < n; ++i) {
            Vector2d a = particles[i]->x.head<2>();
            Vector2d b = particles[(i + 1) % n]->x.head<2>();
            edges.push_back({ a, b });
        }
        return edges;
        };

    auto project = [](const std::vector<std::shared_ptr<Particle>>& pts, const Vector2d& axis, double& min, double& max) {
        min = max = pts[0]->x.head<2>().dot(axis);
        for (auto& p : pts) {
            double proj = p->x.head<2>().dot(axis);
            min = std::min(min, proj);
            max = std::max(max, proj);
        }
        };

    auto edgesA = getEdges(particles);
    auto edgesB = getEdges(other->particles);

    auto checkAxes = [&](const std::vector<std::pair<Vector2d, Vector2d>>& edges) {
        for (auto& edge : edges) {
            Vector2d e = edge.second - edge.first;
            Vector2d axis(-e.y(), e.x());
            axis.normalize();

            double minA, maxA, minB, maxB;
            project(particles, axis, minA, maxA);
            project(other->particles, axis, minB, maxB);

            if (maxA < minB || maxB < minA)
                return false;  // Separating axis found
        }
        return true;
        };

    return checkAxes(edgesA) && checkAxes(edgesB);
}


void Polygon::integratePosition(double timeStep) {
    for (auto& p : particles) {
        if (!p->fixed) {
            p->p = p->x;
            p->x += p->v * timeStep;
        }
    }
}

bool Polygon::isAbove(const std::shared_ptr<Polygon>& other) const {
    // Simple Y-based check: average center of mass
    double thisY = 0.0, otherY = 0.0;
    for (auto& p : particles) thisY += p->x.y();
    for (auto& p : other->particles) otherY += p->x.y();
    thisY /= particles.size();
    otherY /= other->particles.size();
    return thisY > otherY + 0.01; // small bias
}

void Polygon::resolveCollisionsWith(const std::shared_ptr<Polygon>& other, double timeStep) {
    struct MTV {
        Vector2d axis;
        double depth;
    };

    double totalNormalImpulse = 0.0;


    MTV bestMTV;
    bestMTV.depth = std::numeric_limits<double>::infinity();

    auto getEdges = [](const std::vector<std::shared_ptr<Particle>>& particles) {
        std::vector<std::pair<Vector2d, Vector2d>> edges;
        int n = particles.size();
        for (int i = 0; i < n; ++i) {
            Vector2d a = particles[i]->x.head<2>();
            Vector2d b = particles[(i + 1) % n]->x.head<2>();
            edges.push_back({ a, b });
        }
        return edges;
        };

    auto projectPolygon = [](const std::vector<std::shared_ptr<Particle>>& points, const Vector2d& axis, double& min, double& max) {
        min = max = points[0]->x.head<2>().dot(axis);
        for (const auto& p : points) {
            double proj = p->x.head<2>().dot(axis);
            if (proj < min) min = proj;
            if (proj > max) max = proj;
        }
        };

    auto edgesA = getEdges(particles);
    auto edgesB = getEdges(other->particles);

    auto testAxes = [&](const std::vector<std::pair<Vector2d, Vector2d>>& edges) {
        for (const auto& edge : edges) {
            Vector2d e = edge.second - edge.first;
            Vector2d axis = Vector2d(-e.y(), e.x()).normalized();

            double minA, maxA, minB, maxB;
            projectPolygon(particles, axis, minA, maxA);
            projectPolygon(other->particles, axis, minB, maxB);

            double overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if (overlap < 0) return false; // Separating axis -> no collision

            if (overlap < bestMTV.depth) {
                bestMTV.depth = overlap;
                bestMTV.axis = axis;
            }
        }
        return true;
        };

    if (!testAxes(edgesA)) return;
    if (!testAxes(edgesB)) return;

    // At this point, collision confirmed
    // Compute total inverse mass
    double wThis = 1.0 / getTotalMass();
    double wOther = 1.0 / other->getTotalMass();
    double wSum = wThis + wOther;

    if (wSum < 1e-8) return;

    // Direction from this to other
    Vector2d dir = other->getCenter().cast<double>() - getCenter().cast<double>();
    if (dir.dot(bestMTV.axis) < 0)
        bestMTV.axis = -bestMTV.axis;

    Vector2d correction = bestMTV.axis * bestMTV.depth;

    for (auto& p : particles) {
        if (!p->fixed)
            p->x.head<2>() -= correction * (wThis / wSum);
    }
    for (auto& p : other->particles) {
        if (!p->fixed)
            p->x.head<2>() += correction * (wOther / wSum);
    }

    // Apply impulse-based response to transfer momentum
    Vector3d n3d(bestMTV.axis.x(), bestMTV.axis.y(), 0);
    double restitution = 0.0;  // inelastic for realism

    for (auto& pa : particles) {
        if (pa->fixed) continue;
        for (auto& pb : other->particles) {
            if (pb->fixed) continue;

            Vector3d rv = pb->v - pa->v;
            double velAlongNormal = rv.dot(n3d);
            if (velAlongNormal > 0) continue; // separating

            double invMassA = 1.0 / pa->m;
            double invMassB = 1.0 / pb->m;
            double j = -(1.0 + restitution) * velAlongNormal / (invMassA + invMassB);
            totalNormalImpulse += std::abs(j);
            Vector3d impulse = j * n3d;


            pa->v -= impulse * invMassA;
            pb->v += impulse * invMassB;

            // === Friction impulse (constraint-based) ===
            Vector3d tangent = rv - velAlongNormal * n3d;
            if (tangent.norm() > 1e-6) {
                tangent.normalize();

                double relTanVel = rv.dot(tangent);
                double jt = -relTanVel / (invMassA + invMassB);

                // Static + dynamic friction coefficient
                double mu_static = 0.8;
                double mu_dynamic = 0.8;

                double maxFriction = (std::abs(j) > 1e-4 && std::abs(relTanVel) < 0.05)
                    ? mu_static * std::abs(j)
                    : mu_dynamic * std::abs(j);

                double jtClamped = std::clamp(jt, -maxFriction, maxFriction);
                Vector3d frictionImpulse = jtClamped * tangent;

                pa->v -= frictionImpulse * invMassA;
                pb->v += frictionImpulse * invMassB;
            }

        }

    }

}

void Polygon::applyStackingFriction(const std::vector<std::shared_ptr<Polygon>>& others) {
    for (auto& other : others) {
        if (other.get() == this) continue;

        // Check vertical relationship
        double thisY = 0.0, otherY = 0.0;
        for (auto& p : particles) thisY += p->x.y();
        for (auto& p : other->particles) otherY += p->x.y();
        thisY /= particles.size();
        otherY /= other->particles.size();

        // Only apply if THIS is below OTHER
        if (thisY < otherY - 0.01 && isTouching(other)) {
            Vector3d avgVThis = Vector3d::Zero();
            Vector3d avgVOther = Vector3d::Zero();
            for (auto& p : particles) avgVThis += p->v;
            for (auto& p : other->particles) avgVOther += p->v;
            avgVThis /= particles.size();
            avgVOther /= other->particles.size();

            double relVx = avgVOther.x() - avgVThis.x();
            double blend = 0.2; // Tune as needed

            for (auto& p : other->particles)
                if (!p->fixed)
                    p->v.x() -= relVx * blend;
        }
    }
}



double Polygon::getTotalMass() const {
    double mass = 0.0;
    for (const auto& p : particles)
        mass += p->m;
    return mass;
}

double Polygon::computeEffectiveNormalForce(const std::vector<std::shared_ptr<Polygon>>& others) {
    double totalMassAbove = getTotalMass();

    for (const auto& other : others) {
        if (other.get() != this && other->isAbove(shared_from_this())) {
            totalMassAbove += other->getTotalMass();
        }
    }

    return totalMassAbove * 9.8;
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


    const double linearThreshold = 0.1;

    for (auto& p : particles) {
        if (!p->fixed && p->v.norm() < linearThreshold) {
            p->v = Vector3d::Zero();
        }
    }




}

void Polygon::applyGroundFriction(double groundY, const Vector3d& gravity, double timeStep, std::vector<std::shared_ptr<Polygon>> others) {
    double mu = 0.8;

    // Compute total downward force on the polygon
    double totalMass = getTotalMass();
    double normalForce = computeEffectiveNormalForce(others);
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
    // 1. Apply forces (update velocity only)
    applyForces(timeStep, gravity, damping);

    // 2. Integrate velocity into position
    integratePosition(timeStep);

    // 2. Resolve overlaps
    for (int k = 0; k < collisionIters; ++k) {
        for (auto& other : others) {
            if (this < other.get()) {
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

    // 6. Friction
    applyGroundFriction(groundY, gravity, timeStep, others);

    // 7. Stacking friction
    applyStackingFriction(others);



    Vector3d avgV = Vector3d::Zero();
    for (auto& p : particles) avgV += p->v;
    avgV /= particles.size();

    bool atRest = std::abs(avgV.x()) < 0.01 && std::abs(avgV.y()) < 0.01;

    for (auto& p : particles) {
        if (p->v.head<2>().norm() > 0.02) {
            atRest = false;
            break;
        }
    }

    if (atRest) {
        for (auto& p : particles) {
            if (!p->fixed) {
                p->v.setZero();
                p->x.x() = p->p.x();  // full position freeze
            }
        }
    }


}

void drawPolygonOffset(
    const std::vector<std::shared_ptr<Particle>>& particles,
    float offset,
    bool fill,
    const Eigen::Vector4f& color,
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
    glColor4f(color.x(), color.y(), color.z(), color.w());

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

bool Polygon::containsPoint(const Eigen::Vector2f& point, float extraOffset) const {
    using Vec2 = Eigen::Vector2f;

    // Get center
    Vec2 center(0, 0);
    for (auto& p : particles) {
        center += Vec2(p->x.x(), p->x.y());
    }
    center /= particles.size();

    // Compute shifted polygon with same offset used in drawPolygonOffset
    vector<Vec2> shifted;
    float offset = extraOffset;

    for (auto& p : particles) {
        Vec2 pos(p->x.x(), p->x.y());
        Vec2 dir = (pos - center).normalized();
        shifted.push_back(pos + dir * offset);
    }

    // Point-in-polygon test (ray casting)
    bool inside = false;
    int n = shifted.size();
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const Vec2& a = shifted[i];
        const Vec2& b = shifted[j];

        bool intersect = ((a.y() > point.y()) != (b.y() > point.y())) &&
            (point.x() < (b.x() - a.x()) * (point.y() - a.y()) / (b.y() - a.y() + 1e-8f) + a.x());
        if (intersect)
            inside = !inside;
    }

    return inside;
}

float Polygon::getBoundingRadius() const {
    Eigen::Vector2f center = getCenter();
    float maxDistSq = 0.0f;

    for (const auto& p : particles) {
        Eigen::Vector2f pos(p->x.x(), p->x.y());
        float distSq = (pos - center).squaredNorm(); // more efficient than .norm()
        if (distSq > maxDistSq) {
            maxDistSq = distSq;
        }
    }

    return std::sqrt(maxDistSq);
}



void Polygon::applyImpulseAt(const Eigen::Vector2f& worldPoint, const Eigen::Vector2f& impulse2D) {
    for (auto& p : particles) {
        if (!p->fixed) {
            Eigen::Vector2f pos2D(p->x.x(), p->x.y());
            float distance = (pos2D - worldPoint).norm();
            float weight = 1.0f / (1.0f + distance); // Inverse distance weighting

            Eigen::Vector3d impulse3D(impulse2D.x(), impulse2D.y(), 0.0);
            p->v += weight * impulse3D / p->m;
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

    // Approximate size
    float totalLen = 0;
    for (size_t i = 0; i < particles.size(); ++i) {
        const auto& a = particles[i]->x;
        const auto& b = particles[(i + 1) % particles.size()]->x;
        totalLen += (a - b).head<2>().norm(); // 2D length
    }
    float avgLen = totalLen / particles.size();
    float offset = .5 * .1;


    drawPolygonOffset(particles, 0, true, fillColor);
    drawPolygonOffset(particles, 0, false, outlineColor);
}
