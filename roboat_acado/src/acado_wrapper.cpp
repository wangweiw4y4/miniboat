//roboat
#include <roboat_acado/acado_wrapper.h>
//ACADO
#include <acado_toolkit.hpp>

ACADOWrapper::ACADOWrapper(const bool yaw_tracking, const double maxForceLimit,  const double step, const int num_steps, const double d11, const double d22, const double d33, const double m11, const double m22, const double m33, const double aa, const double Q0, const double Q1, const double Q2, const double Q3, const double Q4, const double Q5){

    USING_NAMESPACE_ACADO


    f << dot(x1) == cos(x3)*x4-sin(x3)*x5; 
    f << dot(x2) == sin(x3)*x4+cos(x3)*x5;                   
    f << dot(x3) == x6;
    f << dot(x4) == (m22*x5*x6-d11*x4)/m11+sqrt(2)/2*(u1+u2-u3-u4)/m11;      
    f << dot(x5) == -(m11*x4*x6+d22*x5)/m22+sqrt(2)/2*(u1-u2+u3-u4)/m22;       
    f << dot(x6) == -(m11*x5*x6-m22*x5*x6-d33*x6)/m33+sqrt(2)/2*aa*(u1-u2-u3+u4)/m33;  

    //Function being optimized
    //-----------------------------------

    h << x1;
    h << x2;
    h << x3;
    h << x4;
    h << x5;
    h << x6;
    h << u1;
    h << u2;
    h << u3;
    h << u4;

    Function hN;

    hN << x1;
    hN << x2;
    hN << x3;
    hN << x4;
    hN << x5;
    hN << x6;


    //Weight Matrix
    
    Q = eye<double>(10);
    Q(0,0)=Q0;
    Q(1,1)=Q1;
    if(yaw_tracking){
        Q(2,2)=Q2;
    } else{
        Q(2,2)=0.0001;
    }
    Q(3,3)=Q3;
    Q(4,4)=Q4;
    Q(5,5)=Q5;

    QN = eye<double>(6);
    QN(0,0)=Q(0,0);
    QN(1,1)=Q(1,1);
    QN(2,2)=Q(2,2);
    QN(3,3)=Q(3,3);
    QN(4,4)=Q(4,4);
    QN(5,5)=Q(5,5);


    //ocp optimization
    //-------------------------------------

    ocp = OCP( 0.0, step*num_steps, num_steps );         

    r = DVector(10);
    r.setAll(0.0);

    ocp.minimizeLSQ(Q, h); 
    ocp.minimizeLSQEndTerm(QN, hN);
    
    ocp.subjectTo( f );
   
    //input costraints on speed
    ocp.subjectTo( 0 <= u1 <= maxForceLimit );
    ocp.subjectTo( 0 <= u2 <= maxForceLimit );
    ocp.subjectTo( 0 <= u3 <= maxForceLimit );
    ocp.subjectTo( 0 <= u4 <= maxForceLimit );
}
