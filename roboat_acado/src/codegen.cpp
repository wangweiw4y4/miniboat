//ROS
#include "ros/ros.h"
#include <ros/package.h>
#include <roboat_acado/acado_wrapper.h>

//ACADO
#include <acado_toolkit.hpp>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "codegen");
    ros::NodeHandle n;

    std::string export_path;
    bool yaw_tracking;
    double step, d11, d22, d33, m11, m22, m33, aa,  Q0, Q1, Q2, Q3, Q4, Q5, max_force_limit;
    int num_steps;

    n.getParam("export_path",                         export_path);
    n.getParam("system_dynamics/yaw_tracking",        yaw_tracking);
    n.getParam("system_dynamics/step",                step);
    n.getParam("system_dynamics/num_steps",           num_steps);
    n.getParam("system_dynamics/d11",                 d11);
    n.getParam("system_dynamics/d22",                 d22);
    n.getParam("system_dynamics/d33",                 d33);
    n.getParam("system_dynamics/m11",                 m11);
    n.getParam("system_dynamics/m22",                 m22);
    n.getParam("system_dynamics/m33",                 m33);
    n.getParam("system_dynamics/aa",                  aa);
    //n.getParam("system_dynamics/bb",                  bb);
    n.getParam("system_dynamics/Q0",                  Q0);
    n.getParam("system_dynamics/Q1",                  Q1);
    n.getParam("system_dynamics/Q2",                  Q2);
    n.getParam("system_dynamics/Q3",                  Q3);
    n.getParam("system_dynamics/Q4",                  Q4);
    n.getParam("system_dynamics/Q5",                  Q5);
    n.getParam("system_dynamics/max_force_limit",     max_force_limit);

    ACADOWrapper roboat(yaw_tracking, max_force_limit, step, num_steps, d11, d22, d33, m11, m22, m33, aa, Q0, Q1, Q2, Q3, Q4, Q5);

    USING_NAMESPACE_ACADO

    OCPexport mpc( roboat.ocp );
   
    mpc.set( HESSIAN_APPROXIMATION,         GAUSS_NEWTON      );
    mpc.set( DISCRETIZATION_TYPE,           MULTIPLE_SHOOTING );
    mpc.set( INTEGRATOR_TYPE,               INT_IRK_GL4       );
    mpc.set( NUM_INTEGRATOR_STEPS,          num_steps         );
    mpc.set( QP_SOLVER,                     QP_QPOASES        );
    mpc.set( GENERATE_TEST_FILE,            YES               );
    mpc.set( HOTSTART_QP,                   NO                );
    mpc.set( LEVENBERG_MARQUARDT,           1e-10             );
    mpc.set( GENERATE_MAKE_FILE,            YES               );
    mpc.set( GENERATE_MATLAB_INTERFACE,     YES               );
    mpc.set( GENERATE_SIMULINK_INTERFACE,   YES               );

    if (mpc.exportCode(export_path) != SUCCESSFUL_RETURN)
        exit( EXIT_FAILURE );

    mpc.printDimensionsQP( );
    ROS_INFO("CODEGEN COMPLETE!");

    return EXIT_SUCCESS;
}

