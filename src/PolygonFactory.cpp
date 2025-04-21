#include "PolygonFactory.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace Eigen;

shared_ptr<Polygon> PolygonFactory::CreateRectangle(
    const Vector3d& pos, double width, double height
) {
    double rotation = M_PI / 4.0;
    return make_shared<Polygon>(pos, 4, width, height, rotation);
}

shared_ptr<Polygon> PolygonFactory::CreateRegularPolygon(
    const Vector3d& pos, int numEdges, double width, double height, double rotation
) {
    return make_shared<Polygon>(pos, numEdges, width, height, rotation);
}

vector<shared_ptr<Polygon>> PolygonFactory::CreateStackedRectangles(
    const Vector3d& basePos, int count, double width, double height, double spacing
) {
    vector<shared_ptr<Polygon>> polys;
    for (int i = 0; i < count; ++i) {
        Vector3d pos = basePos + Vector3d(0, i * (height + spacing), 0);
        polys.push_back(CreateRectangle(pos, width, height));
    }
    return polys;
}

vector<shared_ptr<Polygon>> PolygonFactory::CreateWall(
    const Vector3d& basePos, int rows, int cols, double width, double height, double spacing
) {
    vector<shared_ptr<Polygon>> polys;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            Vector3d pos = basePos +
                Vector3d(j * (width + spacing), i * (height + spacing), 0);
            polys.push_back(CreateRectangle(pos, width, height));
        }
    }
    return polys;
}

vector<shared_ptr<Polygon>> PolygonFactory::CreateGridOfPolygons(
    const Vector3d& basePos, int rows, int cols, int numEdges,
    double width, double height, double spacingX, double spacingY
) {
    vector<shared_ptr<Polygon>> polys;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            Vector3d pos = basePos + Vector3d(j * (width + spacingX), i * (height + spacingY), 0);
            polys.push_back(CreateRegularPolygon(pos, numEdges, width, height));
        }
    }
    return polys;
}
