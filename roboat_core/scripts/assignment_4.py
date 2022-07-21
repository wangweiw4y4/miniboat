#!/usr/bin/env python3

import os
import time
import rospy
import math
import numpy as np
from scipy.optimize import linear_sum_assignment
from geometry_msgs.msg import Pose2D
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64

class Test:
    def __init__(self):
        self.testing = False

        self.flag = 0.0

        self.pose_mb1 = Pose2D()
        self.pose_mb2 = Pose2D()
        self.pose_mb3 = Pose2D()
        self.pose_mb4 = Pose2D()

        self.ref_mb1 = Pose2D()
        self.ref_mb2 = Pose2D()
        self.ref_mb3 = Pose2D()
        self.ref_mb4 = Pose2D()

        #I will modify this later to use the swarm node data
        rospy.Subscriber("/miniboat1/shrink_flag", Float64, self.flag_callback)
        rospy.Subscriber("/miniboat1/odometry/filtered", Odometry, self.mb1_callback)
        rospy.Subscriber("/miniboat2/odometry/filtered", Odometry, self.mb2_callback)
        rospy.Subscriber("/miniboat3/odometry/filtered", Odometry, self.mb3_callback)
        rospy.Subscriber("/miniboat4/odometry/filtered", Odometry, self.mb4_callback)

        self.d_mb1_pub = rospy.Publisher("/miniboat1/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb2_pub = rospy.Publisher("/miniboat2/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb3_pub = rospy.Publisher("/miniboat3/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb4_pub = rospy.Publisher("/miniboat4/assignment/reference_pose", Pose2D, queue_size=10)

    def compute_distance(self, x1, y1, x2, y2):
        xc = x1 - x2
        yc = y1 - y2
        distance_to_goal = xc*xc + yc*yc
        return distance_to_goal

    def flag_callback(self, _flag):
        self.flag = _flag.data

    def mb1_callback(self, _mb1):
        self.pose_mb1.x = _mb1.position.position.linear.x
        self.pose_mb1.y = -_mb1.position.position.linear.y

    def mb2_callback(self, _mb2):
        self.pose_mb2.x = _mb2.position.position.linear.x
        self.pose_mb2.y = -_mb2.position.position.linear.y

    def mb3_callback(self, _mb3):
        self.pose_mb3.x = _mb3.position.position.linear.x
        self.pose_mb3.y = -_mb3.position.position.linear.y

    def mb4_callback(self, _mb4):
        self.pose_mb4.x = _mb4.position.position.linear.x
        self.pose_mb4.y = -_mb4.position.position.linear.y

    def desired(self, _assigned):
        self.ref_mb1.x = _assigned[0,0]
        self.ref_mb1.y = _assigned[0,1]
        self.ref_mb2.x = _assigned[1,0]
        self.ref_mb2.y = _assigned[1,1]
        self.ref_mb3.x = _assigned[2,0]
        self.ref_mb3.y = _assigned[2,1]
        self.ref_mb4.x = _assigned[3,0]
        self.ref_mb4.y = _assigned[3,1]
        self.d_mb1_pub.publish(self.ref_mb1)
        self.d_mb2_pub.publish(self.ref_mb2)
        self.d_mb3_pub.publish(self.ref_mb3)
        self.d_mb4_pub.publish(self.ref_mb4)
    
def main():
    rospy.init_node('assignment_4', anonymous=False)
    rate = rospy.Rate(50)
    t = Test()
    goals = np.array([[2.9,1.1],[3.1,1.1],[2.9,0.9],[3.1,0.9]])
    number_of_robots = len(goals)
    distance_squared_matrix = np.zeros([number_of_robots,number_of_robots])
    assigned_goals = np.zeros([number_of_robots,2])
    rospy.logwarn("Start")
    while (not rospy.is_shutdown()):
        if t.flag == 1.0:
            rospy.logwarn("Assignment")
            robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
            for i in range(number_of_robots):
                for j in range(number_of_robots):
                    distance_squared_matrix[i,j] = t.compute_distance(robot_poses[i,0],robot_poses[i,1],goals[j,0],goals[j,1])
            row_ind, col_ind = linear_sum_assignment(distance_squared_matrix)
            for k in range(number_of_robots):
                ind = col_ind[k]
                assigned_goals[k] = goals[ind]
            rospy.logwarn("Assigned")
            t.desired(assigned_goals)
            time.sleep(1)
            t.desired(assigned_goals)
            t.flag = 0.0
        rate.sleep()
    t.testing = False
    rospy.logwarn("Finished")
    rospy.spin()

if __name__ == "__main__":
    try:
        main()
    except rospy.ROSInterruptException:
        pass