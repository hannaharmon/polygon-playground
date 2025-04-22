#pragma once
#ifndef POLYGON_H
#define POLYGON_H

#include <memory>
#include <vector>
#include <Eigen/Dense>

class Particle;
class Spring;

struct Edge {
    std::shared_ptr<Particle> p0;
    std::shared_ptr<Particle> p1;
};

class Polygon : public std::enable_shared_from_this<Polygon> {
public:
    Polygon(const Eigen::Vector3d& pos, int numEdges, double width, double height, double rotation = 0.0);
    void applyForces(double timeStep, const Eigen::Vector3d& gravity, double damping);
    void resolveCollisionsWith(const std::shared_ptr<Polygon>& other, double timeStep);
    void updateVelocities(double timeStep);
    double getTotalMass() const;
    double computeEffectiveNormalForce(const std::vector<std::shared_ptr<Polygon>>& others);
    void applyGroundFriction(double groundY, const Eigen::Vector3d& gravity, double timeStep, std::vector<std::shared_ptr<Polygon>> others);
    void step(
        double timeStep,
        int springIters,
        int collisionIters,
        double groundY,
        const std::vector<std::shared_ptr<Polygon>>& others,
        const Eigen::Vector3d& gravity,
        double damping
    );
    void draw(bool drawParticles = true, bool drawSprings = true, bool drawEdges = true) const;
    bool containsPoint(const Eigen::Vector2f& point, float extraOffset = 0.0f) const;
    bool isAbove(const std::shared_ptr<Polygon>& other) const;
    void applyImpulseAt(const Eigen::Vector2f& worldPoint, const Eigen::Vector2f& impulse2D);
    Eigen::Vector3f defaultOutlineColor = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
    Eigen::Vector3f outlineColor = defaultOutlineColor;
    std::vector<std::shared_ptr<Particle>> particles;
    std::vector<std::shared_ptr<Spring>> springs;

private:
    std::vector<Edge> edges;
    double collisionThickness = 0.08;

    void generateRegularPolygon(const Eigen::Vector3d& center, int numEdges, double width, double height, double rotation);
};

#endif
