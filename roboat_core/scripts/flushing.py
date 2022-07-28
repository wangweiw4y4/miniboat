#!/usr/bin/env python3

import os
import time
import rospy
import math
from roboat_core.msg import Force
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64

#Flushing
class Test:
    def __init__(self):
        self.testing = True

        self.max_thrust = 1.0

        self.force = Force()

        self.force_pub = rospy.Publisher("pid_force", Force, queue_size=1)

    def desired(self, _force):
        self.force = _force
        self.force_pub.publish(self.force)

def main():
    rospy.init_node('flushing', anonymous=False)
    rate = rospy.Rate(50)
    t = Test()
    f = Force()
    f1 = 0
    f2 = 0
    f3 = 0
    f4 = 0
    time.sleep(1)
    rospy.logwarn("Starting")
    if t.testing:
        start_time = rospy.Time.now().secs
        while (rospy.Time.now().secs - start_time) <= 15 and not rospy.is_shutdown():
            f1 = t.max_thrust
            f2 = t.max_thrust
            f3 = 0
            f4 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 30 and not rospy.is_shutdown():
            f3 = t.max_thrust
            f4 = t.max_thrust
            f1 = 0
            f2 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 45 and not rospy.is_shutdown():
            f1 = t.max_thrust
            f2 = t.max_thrust
            f3 = 0
            f4 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
        while (rospy.Time.now().secs - start_time) <= 60 and not rospy.is_shutdown():
            f3 = t.max_thrust
            f4 = t.max_thrust
            f1 = 0
            f2 = 0
            f.data[0] = f1
            f.data[1] = f2
            f.data[2] = f3
            f.data[3] = f4
            t.desired(f)
            rate.sleep()
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
