#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <Eigen/Dense>
#include "Polygon.h"

struct GridCoord {
    int x, y;

    bool operator==(const GridCoord& other) const {
        return x == other.x && y == other.y;
    }
};

namespace std {
    template <>
    struct hash<GridCoord> {
        size_t operator()(const GridCoord& g) const {
            return std::hash<int>()(g.x) ^ (std::hash<int>()(g.y) << 1);
        }
    };
}

class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize) : cellSize(cellSize) {}

    void clear() {
        grid.clear();
    }

    void insert(const std::shared_ptr<Polygon>& poly) {
        auto center = poly->getCenter();
        int cx = static_cast<int>(std::floor(center.x() / cellSize));
        int cy = static_cast<int>(std::floor(center.y() / cellSize));
        GridCoord coord{ cx, cy };
        grid[coord].push_back(poly);
    }

    std::vector<std::shared_ptr<Polygon>> getNearby(const std::shared_ptr<Polygon>& poly) const {
        std::vector<std::shared_ptr<Polygon>> result;
        auto center = poly->getCenter();
        int cx = static_cast<int>(std::floor(center.x() / cellSize));
        int cy = static_cast<int>(std::floor(center.y() / cellSize));

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                GridCoord coord{ cx + dx, cy + dy };
                auto it = grid.find(coord);
                if (it != grid.end()) {
                    for (auto& other : it->second) {
                        if (other.get() != poly.get()) {
                            result.push_back(other);
                        }
                    }
                }
            }
        }
        return result;
    }

private:
    float cellSize;
    std::unordered_map<GridCoord, std::vector<std::shared_ptr<Polygon>>> grid;
};
