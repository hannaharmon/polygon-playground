#pragma once
#ifndef POLYGON_H
#define POLYGON_H

#include <memory>
#include <vector>
#include <Eigen/Dense>

class Particle;
class Spring;

class Polygon {
public:
    Polygon(const Eigen::Vector3d& pos, int numEdges, double width, double height);
    void applyForces(double timeStep, const Eigen::Vector3d& gravity, double damping);
    void satisfyConstraints(int iterations, double groundY);
    void updateVelocities(double timeStep);
    void draw() const;

private:
    std::vector<std::shared_ptr<Particle>> particles;
    std::vector<std::shared_ptr<Spring>> springs;

    void generateRectangularGrid(const Eigen::Vector3d& pos, int rows, int cols, double width, double height);
};

#endif
