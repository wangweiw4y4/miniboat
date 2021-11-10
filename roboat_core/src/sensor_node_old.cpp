//ROS
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Float64MultiArray.h"
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_datatypes.h>
#include <sensor_msgs/Imu.h>
//roboat
#include <roboat_core/SensorFromMicroController.h>
#include <math.h>

//other
#include <Eigen/Dense>

#define SENSOR_BYTE_LENGTH 36
#define XYT_BYTE_LENGTH 12
#define SENSOR_BYTE_LENGTH_USE_GPS SENSOR_BYTE_LENGTH - XYT_BYTE_LENGTH

#define AngularVelocityDirection -1 //for INO055
//#define AngularVelocityDirection 1  //for 3DM-GX5-25
/*3DM-GX5-25 IMU Data Format: IMU[0]~IMU[5]:Roll,Pitch,Yaw,Ax,Ay,Wz*/

float lastX = 0;
float lastY = 0;
char firstposflag = 0;
bool USE_GPS;
double step;
double SAMPLE_T;
int ANGLE_DRIFT = 0;
double KFstate_Pre[6];// x,y,vx,vy,ax,ay

static struct
{
/************Data Area**************/
    float GPSData[3];
    float IMUData[6];
/************Data Area**************/
}SensorFromLowerPacket;

ros::Publisher state_pub;
ros::Publisher pose_pub;
ros::Subscriber rtkgps_sub;

ros::Publisher GPSIMU_pub;


std::vector<double> stateCalc(float GPSData[], float IMUData[])
{
    //state[0-5]:x,y,psai,u,v,r
    std::vector<double> state(6);

    state[0] = GPSData[1]/1000;
    state[1] = GPSData[2]/1000;

    double xdot = (state[0] - lastX)/SAMPLE_T;
    double ydot = (state[1] - lastY)/SAMPLE_T;
    if (xdot>0.2)
    xdot=0.2;
    if (xdot<-0.2)
    xdot=-0.2;
    if (ydot>0.2)
    ydot=0.2;
    if (ydot<-0.2)
    ydot=-0.2;
    if(firstposflag==0)
    {
      xdot=0;
      ydot=0;
      firstposflag=1;
    }

    state[2] = M_PI/180*(IMUData[2] - ANGLE_DRIFT);
    if(state[2] < -M_PI){
        state[2] += 2*M_PI;
    }
    if(state[2] > M_PI){
        state[2] -= 2*M_PI;
    }

    state[3] = cos(state[2])*xdot + sin(state[2])*ydot;
    state[4] = -sin(state[2])*xdot + cos(state[2])*ydot;
    state[5] = AngularVelocityDirection*IMUData[5]*M_PI/180;

    lastX = state[0];
    lastY = state[1];

    return state;
}

   void bno055callback(const sensor_msgs::Imu::ConstPtr& imu_raw)
    {
        double roll, pitch, yaw;
        tf::Matrix3x3 m;

        m = tf::Matrix3x3(
        tf::Quaternion(imu_raw->orientation.x, imu_raw->orientation.y, imu_raw->orientation.z, imu_raw->orientation.w));
        m.getRPY(roll, pitch, yaw);
        ROS_ERROR("bno055: %f,%f,%f", roll, pitch, yaw);

}

void rtkgpsCallback(const std_msgs::Float64MultiArray array)
{
    SensorFromLowerPacket.GPSData[1] = array.data[0];
    SensorFromLowerPacket.GPSData[2] = array.data[1];
}

//callback function for the sensor topic
void imuCallback(roboat_core::SensorFromMicroController SensorData)
{
    if(!USE_GPS){
        memcpy(&SensorFromLowerPacket, &SensorData.SensorDataFromLower[0], SENSOR_BYTE_LENGTH);
    } else{
        //copy all sensor data after time, x and y
        memcpy(&SensorFromLowerPacket.IMUData[0], &SensorData.SensorDataFromLower[XYT_BYTE_LENGTH], SENSOR_BYTE_LENGTH_USE_GPS);
        //copy time
        memcpy(&SensorFromLowerPacket, &SensorData.SensorDataFromLower[0], 4);
    }

    ROS_INFO("[SENSOR NODE]GPS and IMU data:%f,%f,%f,%f,%f,%f\n",SensorFromLowerPacket.GPSData[1],SensorFromLowerPacket.GPSData[2],SensorFromLowerPacket.IMUData[2], SensorFromLowerPacket.IMUData[3],SensorFromLowerPacket.IMUData[4],SensorFromLowerPacket.IMUData[5]);

    std::vector<double> state = stateCalc(SensorFromLowerPacket.GPSData, SensorFromLowerPacket.IMUData);

    //publish state
    std_msgs::Float64MultiArray stateMsg;
    stateMsg.data.resize(6);
    stateMsg.data = state;
    state_pub.publish(stateMsg);
		
	std_msgs::Float64MultiArray GPSIMUMsg;
	GPSIMUMsg.data.resize(9);
	
/*	std::vector<double> gpsimu(9);
	
	for (int i=0;i++;i<3) {
		gpsimu[i]= 3; //SensorFromLowerPacket.GPSData[i];
	}
	
	for (int j=0;j++;j<6) {
		gpsimu[j+3]= 3; //SensorFromLowerPacket.IMUData[j];
	}*/
	
	memcpy(&GPSIMUMsg.data[0],&SensorFromLowerPacket.GPSData[0],36);
	
	GPSIMU_pub.publish(GPSIMUMsg);

    //publish pose representation of state
    geometry_msgs::PoseStamped poseMsg;
    poseMsg.header.frame_id = "world";
    poseMsg.pose.position.x = state[0];
    poseMsg.pose.position.y = state[1];
    poseMsg.pose.orientation = tf::createQuaternionMsgFromYaw(state[2]);
    pose_pub.publish(poseMsg);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "sensor_node");
    ros::NodeHandle n;
    n.param("system_dynamics/step", step, 0.1);
    ros::Rate loop_rate(1/step);
    ros::Time begin = ros::Time::now();

    SAMPLE_T= step;


    if(n.hasParam("IMU/ANGLE_DRIFT")) n.getParam("IMU/ANGLE_DRIFT", ANGLE_DRIFT);

    ROS_ERROR("%d", ANGLE_DRIFT);

    //subscribe the sensor topic and GPS if usingGPS
    ros::Subscriber sub = n.subscribe("sensor", 10, imuCallback);
    ros::Subscriber bno055_sub= n.subscribe("/imu/data", 1, bno055callback);
    //rtkgps_sub = n.subscribe("rtkgps", 100, rtkgpsCallback);

    state_pub = n.advertise<std_msgs::Float64MultiArray>("robot_state", 10);
    GPSIMU_pub = n.advertise<std_msgs::Float64MultiArray>("GPSIMU", 10);
    pose_pub = n.advertise<geometry_msgs::PoseStamped>("robot_pose", 10);

    ros::spin();
    return 0;
} 
