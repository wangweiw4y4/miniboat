#!/usr/bin/env python3

import os
import time
import rospy
import rosbag
import math
from roboat_core.msg import Force
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64

#Zigzag test in yaw
class Test:
    def __init__(self):
        self.testing = True

        self.velocity = Float64()
        self.odom = Odometry()
        self.max_thrust = 0.5

        self.force = Force()

        rospy.Subscriber("odometry/filtered", Odometry, self.odom_callback)

        self.force_pub = rospy.Publisher("pid_force", Force, queue_size=1)


    def odom_callback(self, _odom):
        self.velocity.data = -_odom.twist.twist.angular.z
        self.odom = _odom

    def desired(self, _force):
        self.force = _force
        self.force_pub.publish(self.force)

def main():
    rospy.init_node('zigzag_test_r', anonymous=False)
    rate = rospy.Rate(50)
    t = Test()
    dir_name = os.path.dirname(__file__)
    bag = rosbag.Bag(dir_name + '/zigzag_test_r/90.bag','w')
    p = 90.0
    f = Force()
    f1 = 0
    f2 = 0
    f3 = 0
    f4 = 0
    time.sleep(10)
    rospy.logwarn("Starting")
    if t.testing:
        start_time = rospy.Time.now().secs
        while (rospy.Time.now().secs - start_time) <= 5 and not rospy.is_shutdown():
            rospy.logwarn("Right")
            bag.write('r', t.velocity)
            bag.write('odom', t.odom)
            f1 = (p/100.0)*t.max_thrust
            f2 = 0
            f3 = 0
            f4 = (p/100.0)*t.max_thrust
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 10 and not rospy.is_shutdown():
            rospy.logwarn("Left")
            bag.write('r', t.velocity)
            bag.write('odom', t.odom)
            f1 = 0
            f2 = (p/100.0)*t.max_thrust
            f3 = (p/100.0)*t.max_thrust
            f4 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 15 and not rospy.is_shutdown():
            rospy.logwarn("Right")
            bag.write('r', t.velocity)
            bag.write('odom', t.odom)
            f1 = (p/100.0)*t.max_thrust
            f2 = 0
            f3 = 0
            f4 = (p/100.0)*t.max_thrust
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 20 and not rospy.is_shutdown():
            rospy.logwarn("Left")
            bag.write('r', t.velocity)
            bag.write('odom', t.odom)
            f1 = 0
            f2 = (p/100.0)*t.max_thrust
            f3 = (p/100.0)*t.max_thrust
            f4 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        bag.close()
        f1 = 0
        f2 = 0
        f3 = 0
        f4 = 0
        f.data[0] = f1
        f.data[1] = f2
        f.data[2] = f3
        f.data[3] = f4
        t.desired(f)
        t.testing = False
        rospy.logwarn("Finished")
    rospy.spin()

if __name__ == "__main__":
    try:
        main()
    except rospy.ROSInterruptException:
        pass
