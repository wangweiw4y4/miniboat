#!/usr/bin/env python3

'''
This script is an automated procedure for the expected paper experiments
1. Shape-formation and latching of scathered miniboats into a square (empty center)
2. Reconfiguration (de-latching/formation/latching) into a 4x2 rectangle
3. Point to point motion (center of the pool to the upper left corner)
'''

import os
from re import X
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
        self.shape_msg = Shape()
        self.in_assignment = False

        self.pose_mb1 = Pose2D()
        self.pose_mb2 = Pose2D()
        self.pose_mb3 = Pose2D()
        self.pose_mb4 = Pose2D()

        self.ref_mb1 = Pose2D()
        self.ref_mb2 = Pose2D()
        self.ref_mb3 = Pose2D()
        self.ref_mb4 = Pose2D()

        self.latch = UInt16()

        #rospy.Subscriber("/shape", Shape, self.shape_callback)
        rospy.Subscriber("/miniboat1/shrink_flag", Float64, self.flag_callback)
        '''
        rospy.Subscriber("/miniboat1/odometry/filtered", Odometry, self.mb1_callback)
        rospy.Subscriber("/miniboat2/odometry/filtered", Odometry, self.mb2_callback)
        rospy.Subscriber("/miniboat3/odometry/filtered", Odometry, self.mb3_callback)
        rospy.Subscriber("/miniboat4/odometry/filtered", Odometry, self.mb4_callback)
        '''
        rospy.Subscriber("/miniboat1/boat_state", State, self.mb1_callback)
        rospy.Subscriber("/miniboat2/boat_state", State, self.mb2_callback)
        rospy.Subscriber("/miniboat3/boat_state", State, self.mb3_callback)
        rospy.Subscriber("/miniboat4/boat_state", State, self.mb4_callback)

        self.d_shape_pub = rospy.Publisher("/shape", Shape, queue_size=10)
        self.d_mb1_pub = rospy.Publisher("/miniboat1/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb2_pub = rospy.Publisher("/miniboat2/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb3_pub = rospy.Publisher("/miniboat3/assignment/reference_pose", Pose2D, queue_size=10)
        self.d_mb4_pub = rospy.Publisher("/miniboat4/assignment/reference_pose", Pose2D, queue_size=10)

        self.l_mb1_pub = rospy.Publisher("/miniboat1/latching", UInt16, queue_size=10)
        self.l_mb2_pub = rospy.Publisher("/miniboat2/latching", UInt16, queue_size=10)
        self.l_mb3_pub = rospy.Publisher("/miniboat3/latching", UInt16, queue_size=10)
        self.l_mb4_pub = rospy.Publisher("/miniboat4/latching", UInt16, queue_size=10)

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

    '''def shape_callback(self, _shape):
        self.shape = _shape.shape_code
        self.flag = 0.0
        self.latch_action(2)
        rospy.logwarn("New Shape")'''

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

    def desired(self, _assigned, _diam):
        self.ref_mb1.x      = _assigned[0,0]
        self.ref_mb1.y      = _assigned[0,1]
        self.ref_mb1.theta  = _diam
        self.ref_mb2.x      = _assigned[1,0]
        self.ref_mb2.y      = _assigned[1,1]
        self.ref_mb2.theta  = _diam
        self.ref_mb3.x      = _assigned[2,0]
        self.ref_mb3.y      = _assigned[2,1]
        self.ref_mb3.theta  = _diam
        self.ref_mb4.x      = _assigned[3,0]
        self.ref_mb4.y      = _assigned[3,1]
        self.ref_mb4.theta  = _diam
        self.d_mb1_pub.publish(self.ref_mb1)
        self.d_mb2_pub.publish(self.ref_mb2)
        self.d_mb3_pub.publish(self.ref_mb3)
        self.d_mb4_pub.publish(self.ref_mb4)

    def latch_action(self, _action):
        self.latch.data = _action
        self.l_mb1_pub.publish(self.latch)
        self.l_mb2_pub.publish(self.latch)
        self.l_mb3_pub.publish(self.latch)
        self.l_mb4_pub.publish(self.latch)
    
    def shape_publish(self, _shape):
        self.shape_msg.shape_code = _shape
        self.d_shape_pub.publish(self.shape_msg)

def main():
    rospy.init_node('final_experiment_4', anonymous=False)
    freq = 50
    rate = rospy.Rate(freq)
    t = Test()
    x_center = 2.0
    y_center = 1.5
    x_final = 3.43
    y_final = 0.36
    p2p_time = 60
    inner_diam = 0.18
    outer_diam = 0.32
    goals_0 = np.array([[x_center + 0.12, y_center - 0.12],[x_center + 0.12,y_center + 0.12],[x_center - 0.12,y_center - 0.12],[x_center - 0.12,y_center + 0.12]])
    goals_0b = np.array([[x_center + 0.24, y_center - 0.24],[x_center + 0.24,y_center + 0.24],[x_center - 0.24,y_center - 0.24],[x_center - 0.24,y_center + 0.24]])
    goals_1 = np.array([[x_center + 0.36,y_center],[x_center + 0.12,y_center],[x_center - 0.12,y_center],[x_center - 0.36,y_center]])
    goals_1b = np.array([[x_center + 0.54,y_center],[x_center + 0.24,y_center],[x_center - 0.24,y_center],[x_center - 0.54,y_center]])
    t.shape = 0
    t.shape = 0
    number_of_robots = len(goals_0)
    distance_squared_matrix = np.zeros([number_of_robots,number_of_robots])
    goals = np.zeros([number_of_robots,2])
    goalsb = np.zeros([number_of_robots,2])
    assigned_goals = np.zeros([number_of_robots,2])
    miniboat_list = np.zeros([number_of_robots])
    state = 0
    time.sleep(5)
    rospy.logwarn("Start")
    while (not rospy.is_shutdown()):
        if state == 0: #publish square shape to begin
            t.latch_action(2)
            t.shape_publish(t.shape)
            time.sleep(1)
            t.latch_action(2)
            t.shape_publish(t.shape)
            state = 1
        if state == 1: #configurate into a square
            rospy.logwarn("Begin Square")
            goals = goals_0
            goalsb = goals_0b
            while (not rospy.is_shutdown()):
                if t.flag == 1.0:
                    rospy.logwarn("Square Assignment")
                    robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                    for i in range(number_of_robots):
                        for j in range(number_of_robots):
                            distance_squared_matrix[i,j] = t.compute_distance(robot_poses[i,0],robot_poses[i,1],goals[j,0],goals[j,1])
                    row_ind, col_ind = linear_sum_assignment(distance_squared_matrix)
                    for k in range(number_of_robots):
                        ind = col_ind[k]
                        assigned_goals[k] = goalsb[ind]
                        miniboat_list[ind] = k
                    rospy.logwarn("Square Assigned Out")
                    t.desired(assigned_goals, outer_diam)
                    time.sleep(1)
                    t.desired(assigned_goals, outer_diam)
                    rospy.logwarn(miniboat_list + 1)
                    rospy.logwarn(assigned_goals)
                    t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                    while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                        robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                        t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                        rate.sleep()
                    rospy.logwarn("Square first positions")
                    if t.flag == 1.0:
                        for k in range(number_of_robots):
                            ind = col_ind[k]
                            assigned_goals[k] = goals[ind]
                            miniboat_list[ind] = k
                        rospy.logwarn("Square Assigned In")
                        t.desired(assigned_goals, inner_diam)
                        time.sleep(1)
                        t.desired(assigned_goals, inner_diam)
                        rospy.logwarn(miniboat_list + 1)
                        rospy.logwarn(assigned_goals)            
                        t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                        while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                            robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                            t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                            rate.sleep()
                        rospy.logwarn("Square second positions")
                        if t.flag == 1.0:
                            t.latch_action(1)
                            time.sleep(1)
                            t.latch_action(1)
                            rospy.logwarn("Square Latching")
                    t.flag = 0.0
                    state = 2
                rate.sleep()
        if state == 2:
            rospy.logwarn("Remain in Square")
            time.sleep(10)
            state = 3
            rospy.logwarn("Delatch")
            t.shape = 1
            t.latch_action(2)
            t.shape_publish(t.shape)
            time.sleep(1)
            t.latch_action(2)
            t.shape_publish(t.shape)
        if state == 3:
            rospy.logwarn("Begin Rectangle")
            goals = goals_1
            goalsb = goals_1b
            while (not rospy.is_shutdown()):
                if t.flag == 1.0:
                    rospy.logwarn("Rectangle Assignment")
                    robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                    for i in range(number_of_robots):
                        for j in range(number_of_robots):
                            distance_squared_matrix[i,j] = t.compute_distance(robot_poses[i,0],robot_poses[i,1],goals[j,0],goals[j,1])
                    row_ind, col_ind = linear_sum_assignment(distance_squared_matrix)
                    for k in range(number_of_robots):
                        ind = col_ind[k]
                        assigned_goals[k] = goalsb[ind]
                        miniboat_list[ind] = k
                    rospy.logwarn("Rectangle Assigned Out")
                    t.desired(assigned_goals, outer_diam)
                    time.sleep(1)
                    t.desired(assigned_goals, outer_diam)
                    rospy.logwarn(miniboat_list + 1)
                    rospy.logwarn(assigned_goals)
                    t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                    while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                        robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                        t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                        rate.sleep()
                    rospy.logwarn("Rectangle first positions")
                    if t.flag == 1.0:
                        for k in range(number_of_robots):
                            ind = col_ind[k]
                            assigned_goals[k] = goals[ind]
                            miniboat_list[ind] = k
                        rospy.logwarn("Rectangle Assigned In")
                        t.desired(assigned_goals, inner_diam)
                        time.sleep(1)
                        t.desired(assigned_goals, inner_diam)
                        rospy.logwarn(miniboat_list + 1)
                        rospy.logwarn(assigned_goals)            
                        t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                        while ((t.in_assignment == False) and (t.flag == 1.0) and (not rospy.is_shutdown())):
                            robot_poses = np.array([[t.pose_mb1.x,t.pose_mb1.y],[t.pose_mb2.x,t.pose_mb2.y],[t.pose_mb3.x,t.pose_mb3.y],[t.pose_mb4.x,t.pose_mb4.y]])
                            t.in_assignment = t.is_in_assigned(robot_poses,assigned_goals,number_of_robots)
                            rate.sleep()
                        rospy.logwarn("Rectangle second positions")
                        if t.flag == 1.0:
                            t.latch_action(1)
                            time.sleep(1)
                            t.latch_action(1)
                            rospy.logwarn("Rectangle Latching")
                    t.flag = 0.0
                    state = 4
                rate.sleep()
        if state == 4:
            time.sleep(5)
            rospy.logwarn("Begin Motion")
            start_time = rospy.Time.now().secs
            timer = 0
            ap = (x_final - x_center)/p2p_time
            bp = (y_final - y_center)/p2p_time
            while (rospy.Time.now().secs - start_time) <= p2p_time and not rospy.is_shutdown():
                timer += 1/freq
                x_pos = ap*timer + x_center
                y_pos = bp*timer + y_center
                goals = np.array([[x_pos + 0.33,y_pos],[x_pos + 0.11,y_pos],[x_pos - 0.11,y_pos],[x_pos - 0.33,y_pos]])
                for k in range(number_of_robots):
                    ind = col_ind[k]
                    assigned_goals[k] = goals[ind]
                    miniboat_list[ind] = k
                t.desired(assigned_goals, inner_diam)
                rate.sleep
            state = 5
        if state == 5:
            t.testing = False
            rospy.logwarn("Finished")
        rate.sleep()
    rospy.spin()

if __name__ == "__main__":
    try:
        main()
    except rospy.ROSInterruptException:
        pass