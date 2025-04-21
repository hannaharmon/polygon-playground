#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Polygon.h"

class SceneManager {
public:
    using SceneFunc = std::function<std::vector<std::shared_ptr<Polygon>>()>;

    void RegisterScene(int key, SceneFunc func);
    void LoadScene(int key);
    std::vector<std::shared_ptr<Polygon>>& GetPolygons();

private:
    std::unordered_map<int, SceneFunc> scenes;
    std::vector<std::shared_ptr<Polygon>> currentPolygons;
};
