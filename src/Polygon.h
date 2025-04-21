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

class Polygon {
public:
    Polygon(const Eigen::Vector3d& pos, int numEdges, double width, double height, double rotation = 0.0);
    void applyForces(double timeStep, const Eigen::Vector3d& gravity, double damping);
    void resolveCollisionsWith(const std::shared_ptr<Polygon>& other, double timeStep);
    void updateVelocities(double timeStep);
    double getTotalMass() const;
    void applyGroundFriction(double groundY, const Eigen::Vector3d& gravity, double timeStep);
    void step(
        double timeStep,
        int springIters,
        int collisionIters,
        double groundY,
        const std::vector<std::shared_ptr<Polygon>>& others,
        const Eigen::Vector3d& gravity,
        double damping
    );
    void draw(bool drawParticles = false, bool drawSprings = false, bool drawEdges = true) const;

private:
    std::vector<Edge> edges;
    std::vector<std::shared_ptr<Particle>> particles;
    std::vector<std::shared_ptr<Spring>> springs;
    double collisionThickness = 0.08;

    void generateRegularPolygon(const Eigen::Vector3d& center, int numEdges, double width, double height, double rotation);
};

#endif
