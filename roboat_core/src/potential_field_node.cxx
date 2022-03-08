/** ----------------------------------------------------------------------------
 * @file:     roboat_pf.cpp
 * @date:     July 18, 2021
 * @datemod:  Nov 12, 2021
 * @author:   Alejandro Gonzalez-Garcia
 * @email:    alexglzg97@gmail.com
 * 
 * @brief: Potential fields algortithm for distributed formation control. 
 * ---------------------------------------------------------------------------*/

#include <roboat_core/potential_field.h>

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "pf_node");
    ros::NodeHandle nh;
    PotentialField potentialField(nh);
    int rate = 100;
    ros::Rate loop_rate(rate);
    ros::Duration(5).sleep();
    //force parameters
    forceparameters_t fp = { potentialField.attractive_par_1, potentialField.attractive_par_2, 1.0, 0.2, 1.0 }; //later 3 parameters are irrelevant to the attractive force
    //define shape
    potentialField.region = potentialField.multi_region*potentialField.target_region; //half of the initial square side
    potentialField.r0 = potentialField.multi_r0*potentialField.target_r0; //initial r0
    potentialField.stf = potentialField.multi_stf*potentialField.target_stf; //initial lattice force coefficient
    potentialField.srf = potentialField.multi_srf*potentialField.target_srf; //initial repulsive force coefficient
    //create the shape
    std::vector<point_t> shape_points = potentialField.updateShape();
    polygon_t shape(shape_points, fp);
    //establish the shrinking rates
    potentialField.region_rate = std::abs(potentialField.target_region - potentialField.region)/(potentialField.shrink_time*rate);
    potentialField.r0_rate = std::abs(potentialField.target_r0 - potentialField.r0)/(potentialField.shrink_time*rate);
    potentialField.srf_rate = std::abs(potentialField.target_stf - potentialField.stf)/(potentialField.shrink_time*rate);
    potentialField.stf_rate = std::abs(potentialField.target_srf - potentialField.srf)/(potentialField.shrink_time*rate);
    potentialField.counter_thresh = potentialField.inside_time*rate;
    potentialField.shape_vertexes = shape_points;
    
    while (ros::ok())
    {
        potentialField.timeStep(shape);
        ros::spinOnce();
        loop_rate.sleep();
    }

    return 0;
}
