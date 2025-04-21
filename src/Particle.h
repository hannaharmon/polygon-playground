#pragma once
#ifndef Particle_H
#define Particle_H

#include <vector>
#include <memory>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

class Shape;

class Particle
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	
	Particle();
	virtual ~Particle();
	
	double m; // mass
	double d; // damping
	Eigen::Vector3d x;  // position
	Eigen::Vector3d p;  // previous position
	Eigen::Vector3d v;  // velocity
	bool fixed;

};

#endif
