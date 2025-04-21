#pragma once

#include <memory>
#include <vector>
#include <Eigen/Dense>
#include "Polygon.h"

class PolygonFactory {
public:
    static std::shared_ptr<Polygon> CreateRectangle(
        const Eigen::Vector3d& pos,
        double width,
        double height
    );

    static std::shared_ptr<Polygon> CreateRegularPolygon(
        const Eigen::Vector3d& pos,
        int numEdges,
        double width,
        double height,
        double rotation = 0.0
    );

    static std::vector<std::shared_ptr<Polygon>> CreateStackedRectangles(
        const Eigen::Vector3d& basePos,
        int count,
        double width,
        double height,
        double spacing = 0.0
    );

    static std::vector<std::shared_ptr<Polygon>> CreateWall(
        const Eigen::Vector3d& basePos,
        int rows,
        int cols,
        double width,
        double height,
        double spacing = 0.0
    );

    static std::vector<std::shared_ptr<Polygon>> CreateGridOfPolygons(
        const Eigen::Vector3d& basePos,
        int rows,
        int cols,
        int numEdges,
        double width,
        double height,
        double spacingX = 0.0,
        double spacingY = 0.0
    );
};
