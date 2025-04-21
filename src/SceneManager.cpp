#include "SceneManager.h"

void SceneManager::RegisterScene(int key, SceneFunc func) {
    scenes[key] = func;
}

void SceneManager::LoadScene(int key) {
    auto it = scenes.find(key);
    if (it != scenes.end()) {
        currentPolygons = it->second(); // Replace with fresh polygons
    }
}

std::vector<std::shared_ptr<Polygon>>& SceneManager::GetPolygons() {
    return currentPolygons;
}
