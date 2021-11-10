#ifndef ACADOWRAPPER_H
#define	ACADOWRAPPER_H

#include <acado_toolkit.hpp>

class ACADOWrapper
{
private:
    ACADO::DifferentialState x1, x2, x3, x4, x5,x6;
    ACADO::Control u1, u2, u3, u4;
    ACADO::DMatrix Q;
    ACADO::DMatrix QN;
    ACADO::Function h;
    ACADO::DifferentialEquation f;
    ACADO::DVector state, r;

public:
    ACADOWrapper(const bool yaw_tracking, const double maxForceLimit, const double step, const int num_steps, const double d11, const double d22, const double d33, const double m11, const double m22, const double m33, const double aa, const double Q0, const double Q1, const double Q2, double Q3, double Q4, double Q5);
    ACADO::OCP ocp;
};
#endif
