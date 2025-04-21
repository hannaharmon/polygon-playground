#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Particle.h"

using namespace std;

Particle::Particle() :
	r(.08),
	m(1.0),
	x(0.0, 0.0, 0.0),
	v(0.0, 0.0, 0.0),
	fixed(false)
{
	
}

Particle::~Particle()
{
}

void Particle::tare()
{
	x0 = x;
	v0 = v;
}

void Particle::reset()
{
	x = x0;
	v = v0;
}

void Particle::draw(shared_ptr<MatrixStack> MV, const shared_ptr<Program> prog) const
{

}
