# polygon playground

**polygon playground** is a physics-based 2D interaction sandbox inspired by Boom Blox Bash Party. Built using modern C++ and OpenGL, it allows users to create, manipulate, and destroy polygon structures with a variety of tools.

---

## ⇨ features

- Real-time interaction with 2D polygon objects
- Slingshot and Grab mechanics
- Support for rotation, resizing, selection, and flicking
- Custom pixel-style UI and tool icons
- Minimalist OpenGL-based rendering with a grid background

---

## ⇨ setup

### ❑ dependencies

Before building, ensure the following libraries are installed:

- [GLFW](https://www.glfw.org/)
- [GLEW](http://glew.sourceforge.net/)
- [OpenGL](https://www.opengl.org/)
- [GLM](https://github.com/g-truc/glm)
- [Eigen](https://eigen.tuxfamily.org/)
- `stb_image.h` (included)
- CMake (version 3.10 or higher)

You can install the dependencies using your system's package manager:

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install cmake libglfw3-dev libglew-dev libglm-dev libeigen3-dev
```

**macOS (Homebrew):**
```bash
brew install cmake glfw glew glm eigen
```

**Windows:**
- Use [vcpkg](https://github.com/microsoft/vcpkg) or download prebuilt binaries.
- Be sure to add include/library paths in your CMake GUI.

---

## ⇨ building with CMake

### ❑ option 1: Command Line

```bash
git clone https://github.com/your-username/polygon-playground.git
cd polygon-playground
mkdir build
cd build
cmake ..
cmake --build .
```

### ❑ option 2: CMake GUI (Windows/macOS)

1. Open **CMake GUI**
2. Set **"Source"** to the root of this project
3. Set **"Build"** to a new `/build` folder
4. Click **Configure**
5. Choose your compiler (e.g., Visual Studio, GCC, Clang)
6. Click **Generate**
7. Open the generated project in your IDE or click **Open Project**

---

## ⇨ run

After building, run the executable:
```bash
./PolygonPlayground
```
(On Windows: `PolygonPlayground.exe`)

---
