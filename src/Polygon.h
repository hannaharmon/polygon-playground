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
    Polygon(const Eigen::Vector3d& pos, int numEdges, double width, double height);
    void applyForces(double timeStep, const Eigen::Vector3d& gravity, double damping);
	void satisfyConstraints(int springIters, int collisionIters, double groundY, const std::vector<std::shared_ptr<Polygon>>& others);
    void resolveCollisionsWith(const std::shared_ptr<Polygon>& other);
    void updateVelocities(double timeStep);
    void draw(bool drawParticles = true, bool drawSprings = true, bool drawEdges = true) const;

private:
    std::vector<Edge> edges;
    std::vector<std::shared_ptr<Particle>> particles;
    std::vector<std::shared_ptr<Spring>> springs;
    double collisionThickness = 0.1;

    void generateRectangularGrid(const Eigen::Vector3d& pos, int rows, int cols, double width, double height);
};

#endif
