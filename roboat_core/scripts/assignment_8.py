#!/usr/bin/env python3

import os
import time
import rospy
import math
import numpy as np
from scipy.optimize import linear_sum_assignment
from geometry_msgs.msg import Pose2D
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64, UInt16
from roboat_msgs.msg import Shape
from roboat_core.msg import State

class Test:
    def __init__(self):
        self.testing = False

        self.flag = 0.0

        self.shape = 0
        #self.shape_msg = Shape()
        self.in_assignment = False

        self.pose_mb1 = Pose2D()
        self.pose_mb2 = Pose2D()
        self.pose_mb3 = Pose2D()
        self.pose_mb4 = Pose2D()
        self.pose_mb5 = Pose2D()
        self.pose_mb6 = Pose2D()
        self.pose_mb7 = Pose2D()
        self.pose_mb8 = Pose2D()

        self.ref_mb1 = Pose2D()
        self.ref_mb2 = Pose2D()
        self.ref_mb3 = Pose2D()
        self.ref_mb4 = Pose2D()
        self.ref_mb5 = Pose2D()
        self.ref_mb6 = Pose2D()
        self.ref_mb7 = Pose2D()
        self.ref_mb8 = Pose2D()

        self.latch = UInt16()

        #I will modify this later to use the swarm node data
        rospy.Subscriber("/shape", Shape, self.shape_callback)
        rospy.Subscriber("/miniboat1/shrink_flag", Float64, self.flag_callback)
        '''
        rospy.Subscriber("/miniboat1/odometry/filtered", Odometry, self.mb1_callback)
        rospy.Subscriber("/miniboat2/odometry/filtered", Odometry, self.mb2_callback)
        rospy.Subscriber("/miniboat3/odometry/filtered", Odometry, self.mb3_callback)
        rospy.Subscriber("/miniboat4/odometry/filtered", Odometry, self.mb4_callback)
        rospy.Subscriber("/miniboat5/odometry/filtered", Odometry, self.mb5_callback)
        rospy.Subscriber("/miniboat6/odometry/filtered", Odometry, self.mb6_callback)
        rospy.Subscriber("/miniboat7/odometry/filtered", Odometry, self.mb7_callback)
        rospy.Subscriber("/miniboat8/odometry/filtered", Odometry, self.mb8_callback)
        '''
        rospy.Subscriber("/miniboat1/boat_state", State, self.mb1_callback)
        rospy.Subscriber("/miniboat2/boat_state", State, self.mb2_callback)
        rospy.Subscriber("/miniboat3/boat_state", State, self.mb3_callback)
        rospy.Subscriber("/miniboat4/boat_state", State, self.mb4_callback)
        rospy.Subscriber("/miniboat5/boat_state", State, self.mb5_callback)
        rospy.Subscriber("/miniboat6/boat_state", State, self.mb6_callback)
        rospy.Subscriber("/miniboat7/boat_state", State, self.mb7_callback)
        rospy.Subscriber("/miniboat8/boat_state", State, self.mb8_callback)

        #self.d_shape_pub = rospy.Publisher("/shape", Shape, queue_size=10)
        self.d_mb1_pub = rospy.Publisher("/miniboat1/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb2_pub = rospy.Publisher("/miniboat2/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb3_pub = rospy.Publisher("/miniboat3/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb4_pub = rospy.Publisher("/miniboat4/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb5_pub = rospy.Publisher("/miniboat5/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb6_pub = rospy.Publisher("/miniboat6/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb7_pub = rospy.Publisher("/miniboat7/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb8_pub = rospy.Publisher("/miniboat8/assignment/reference_pose", Pose2D, queue_size=10)

        self.l_mb1_pub = rospy.Publisher("/miniboat1/latching", UInt16, queue_size=10)
        self.l_mb2_pub = rospy.Publisher("/miniboat2/latching", UInt16, queue_size=10)
        self.l_mb3_pub = rospy.Publisher("/miniboat3/latching", UInt16, queue_size=10)
        self.l_mb4_pub = rospy.Publisher("/miniboat4/latching", UInt16, queue_size=10)
        self.l_mb5_pub = rospy.Publisher("/miniboat5/latching", UInt16, queue_size=10)
        self.l_mb6_pub = rospy.Publisher("/miniboat6/latching", UInt16, queue_size=10)
        self.l_mb7_pub = rospy.Publisher("/miniboat7/latching", UInt16, queue_size=10)
        self.l_mb8_pub = rospy.Publisher("/miniboat8/latching", UInt16, queue_size=10)

    def compute_distance(self, x1, y1, x2, y2):
        xc = x1 - x2
        yc = y1 - y2
        distance_to_goal = xc*xc + yc*yc
        return distance_to_goal

    def distance_to_assigned_position(self, x1, y1, x2, y2):
        xc = x1 - x2
        yc = y1 - y2
        distance_to_goal = np.sqrt(xc*xc + yc*yc)
        return distance_to_goal
    
    def is_in_assigned(self, _robot_poses, _assigned,_number_of_robots):
        in_position = True
        for i in range(_number_of_robots):
            dist = self.distance_to_assigned_position(_robot_poses[i][0],_robot_poses[i][1],_assigned[i][0],_assigned[i][1])
            if dist <= 0.05 and in_position == True:
                in_position = True
            else:
                in_position = False
        return in_position

    def shape_callback(self, _shape):
        self.shape = _shape.shape_code
        self.flag = 0.0
        self.latch_action(2)
        time.sleep(1)
        self.latch_action(2)
        rospy.logwarn("New Shape")

    def flag_callback(self, _flag):
        self.flag = _flag.data

    def mb1_callback(self, _mb1):
        self.pose_mb1.x = _mb1.data[0]#_mb1.pose.pose.position.x
        self.pose_mb1.y = _mb1.data[1]#-_mb1.pose.pose.position.y

    def mb2_callback(self, _mb2):
        self.pose_mb2.x = _mb2.data[0]#_mb2.pose.pose.position.x
        self.pose_mb2.y = _mb2.data[1]#-_mb2.pose.pose.position.y

    def mb3_callback(self, _mb3):
        self.pose_mb3.x = _mb3.data[0]#_mb3.pose.pose.position.x
        self.pose_mb3.y = _mb3.data[1]#-_mb3.pose.pose.position.y

    def mb4_callback(self, _mb4):
        self.pose_mb4.x = _mb4.data[0]#_mb4.pose.pose.position.x
        self.pose_mb4.y = _mb4.data[1]#-_mb4.pose.pose.position.y

    def mb5_callback(self, _mb5):
        self.pose_mb5.x = _mb5.data[0]#_mb5.pose.pose.position.x
        self.pose_mb5.y = _mb5.data[1]#-_mb5.pose.pose.position.y

    def mb6_callback(self, _mb6):
        self.pose_mb6.x = _mb6.data[0]#_mb6.pose.pose.position.x
        self.pose_mb6.y = _mb6.data[1]#-_mb6.pose.pose.position.y

    def mb7_callback(self, _mb7):
        self.pose_mb7.x = _mb7.data[0]#_mb7.pose.pose.position.x
        self.pose_mb7.y = _mb7.data[1]#-_mb7.pose.pose.position.y

    def mb8_callback(self, _mb8):
        self.pose_mb8.x = _mb8.data[0]#_mb8.pose.pose.position.x
        self.pose_mb8.y = _mb8.data[1]#-_mb8.pose.pose.position.y

    def desired(self, _assigned):
        self.ref_mb1.x = _assigned[0,0]
        self.ref_mb1.y = _assigned[0,1]
        self.ref_mb2.x = _assigned[1,0]
        self.ref_mb2.y = _assigned[1,1]
        self.ref_mb3.x = _assigned[2,0]
        self.ref_mb3.y = _assigned[2,1]
        self.ref_mb4.x = _assigned[3,0]
        self.ref_mb4.y = _assigned[3,1]
        self.ref_mb5.x = _assigned[4,0]
        self.ref_mb5.y = _assigned[4,1]
        self.ref_mb6.x = _assigned[5,0]
        self.ref_mb6.y = _assigned[5,1]
        self.ref_mb7.x = _assigned[6,0]
        self.ref_mb7.y = _assigned[6,1]
        self.ref_mb8.x = _assigned[7,0]
        self.ref_mb8.y = _assigned[7,1]
        self.d_mb1_pub.publish(self.ref_mb1)
        self.d_mb2_pub.publish(self.ref_mb2)
        self.d_mb3_pub.publish(self.ref_mb3)
        self.d_mb4_pub.publish(self.ref_mb4)
        self.d_mb5_pub.publish(self.ref_mb5)
        self.d_mb6_pub.publish(self.ref_mb6)
        self.d_mb7_pub.publish(self.ref_mb7)
        self.d_mb8_pub.publish(self.ref_mb8)

    def latch_action(self, _action):
        self.latch.data = _action
        self.l_mb1_pub.publish(self.latch)
        self.l_mb2_pub.publish(self.latch)
        self.l_mb3_pub.publish(self.latch)
        self.l_mb4_pub.publish(self.latch)
        self.l_mb5_pub.publish(self.latch)
        self.l_mb6_pub.publish(self.latch)
        self.l_mb7_pub.publish(self.latch)
        self.l_mb8_pub.publish(self.latch)

def main():
    rospy.init_node('assignment_8', anonymous=False)
    rate = rospy.Rate(50)
    t = Test()
    x_center = 2.0
    y_center = 1.5
    goals_0 = np.array([[x_center + 0.22, y_center - 0.22],[x_center + 0.22,y_center],[x_center + 0.22,y_center + 0.22],[x_center,y_center - 0.22],[x_center,y_center + 0.22],[x_center - 0.22,y_center - 0.22],[x_center - 0.22,y_center],[x_center - 0.22,y_center + 0.22]])
    goals_1 = np.array([[x_center + 0.36,y_center - 0.12],[x_center + 0.36,y_center + 0.12],[x_center + 0.12,y_center - 0.12],[x_center + 0.12,y_center + 0.12],[x_center - 0.12,y_center - 0.12],[x_center - 0.12,y_center + 0.12],[x_center - 0.36,y_center - 0.12],[x_center - 0.36,y_center + 0.12]])
    goals_0b = np.array([[x_center + 0.32, y_center - 0.32],[x_center + 0.32,y_center],[x_center + 0.32,y_center + 0.32],[x_center,y_center - 0.32],[x_center,y_center + 0.32],[x_center - 0.32,y_center - 0.32],[x_center - 0.32,y_center],[x_center - 0.32,y_center + 0.32]])
    goals_1b = np.array([[x_center + 0.56,y_center - 0.32],[x_center + 0.56,y_center + 0.32],[x_center + 0.22,y_center - 0.22],[x_center + 0.22,y_center + 0.22],[x_center - 0.22,y_center - 0.22],[x_center - 0.22,y_center + 0.22],[x_center - 0.56,y_center - 0.32],[x_center - 0.56,y_center + 0.12]])
    if t.shape == 0:
        goals = goals_0
        goalsb = goals_0b
    if t.shape == 1:
        goals = goals_1
        goalsb = goals_1b
    number_of_robots = len(goals)
    distance_squared_matrix = np.zeros([number_of_robots,number_of_robots])
    assigned_goals = np.zeros([number_of_robots,2])
    miniboat_list = np.zeros([number_of_robots])
    rospy.logwarn("Start")
    while (not rospy.is_shutdown()):
        if t.shape == 0:
            goals = goals_0
            goalsb = goals_0b
        if t.shape == 1:
            goals = goals_1
            goalsb = goals_1b
        if t.flag == 1.0:
            rospy.logwarn("Assignment")
            robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y],[t.pose_mb5.x,t.pose_mb5.y],[t.pose_mb6.x,t.pose_mb6.y],[t.pose_mb7.x,t.pose_mb7.y],[t.pose_mb8.x,t.pose_mb8.y]])
            for i in range(number_of_robots):
                for j in range(number_of_robots):
                    distance_squared_matrix[i,j] = t.compute_distance(robot_poses[i,0],robot_poses[i,1],goals[j,0],goals[j,1])
            row_ind, col_ind = linear_sum_assignment(distance_squared_matrix)
            for k in range(number_of_robots):
                ind = col_ind[k]
                assigned_goals[k] = goalsb[ind]
                miniboat_list[ind] = k
            rospy.logwarn("Assigned Out")
            t.desired(assigned_goals)
            time.sleep(1)
            t.desired(assigned_goals)
            rospy.logwarn(miniboat_list + 1)
            rospy.logwarn(assigned_goals)
            t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
            while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y],[t.pose_mb5.x,t.pose_mb5.y],[t.pose_mb6.x,t.pose_mb6.y],[t.pose_mb7.x,t.pose_mb7.y],[t.pose_mb8.x,t.pose_mb8.y]])
                t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                rate.sleep()
            if t.flag == 1.0:
                for k in range(number_of_robots):
                    ind = col_ind[k]
                    assigned_goals[k] = goals[ind]
                    miniboat_list[ind] = k
                rospy.logwarn("Assigned In")
                t.desired(assigned_goals)
                time.sleep(1)
                t.desired(assigned_goals)
                rospy.logwarn(miniboat_list + 1)
                rospy.logwarn(assigned_goals)            
                t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                    robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y],[t.pose_mb5.x,t.pose_mb5.y],[t.pose_mb6.x,t.pose_mb6.y],[t.pose_mb7.x,t.pose_mb7.y],[t.pose_mb8.x,t.pose_mb8.y]])
                    t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                    rate.sleep()
                if t.flag == 1.0:
                    t.latch_action(1)
                    time.sleep(1)
                    t.latch_action(1)
                    rospy.logwarn("Latching")
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