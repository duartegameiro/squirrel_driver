/****************************************************************
 *
 * Copyright (c) 2016
 *
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: SQUIRREL
 * ROS stack name: squirrel_robotino
 * ROS package name: robotino_controller_configuration_gazebo
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Author: Nadia Hammoudeh Garcia, email:nadia.hammoudeh.garcia@ipa.fraunhofer.de
 * Supervised by:  Nadia Hammoudeh Garcia, email:nadia.hammoudeh.garcia@ipa.fraunhofer.de
 *
 * Date of creation: Juli 2016
 * ToDo:
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Fraunhofer Institute for Manufacturing
 *       Engineering and Automation (IPA) nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/
#include <ros/ros.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Float64.h>
#include <geometry_msgs/Twist.h>
#include <tf/transform_listener.h>
#include <sensor_msgs/JointState.h>
#include <chrono>
#include <thread>
#include <uibk_robot_driver/base_controller.hpp>

using namespace std;


class ArmController{
public:
  ros::NodeHandle nh_;
  ros::Publisher  cmdVelPub_ ;
  ros::Publisher  ArmPosCommandPub_ ;
  ros::Publisher  JointStatesPub_ ;
  void PosCommandSub_cb(const std_msgs::Float64MultiArray msg);
  virtual void JointStatePub();
  vector<double> computeDerivative(vector<int> v1, vector<int> v2, double timeStep);
  vector<double> computeDerivative(vector<double> v1, vector<double> v2, double timeStep);
  std::shared_ptr<BaseController> base = std::make_shared<BaseController>(nh_, 20);
  vector<double> current_pose;
};

void ArmController::PosCommandSub_cb(const std_msgs::Float64MultiArray msg){

  current_pose = base->getCurrentState();

  double cx = current_pose.at(0);
  double cy = current_pose.at(1);
  double ctheta = current_pose.at(2);

  base->moveBase( cx+msg.data[0], cy+msg.data[1], ctheta+msg.data[2]);
  ArmPosCommandPub_ = nh_.advertise<std_msgs::Float64MultiArray>("joint_group_position_controller/command", 1);
  std_msgs::Float64MultiArray joint_pos;
  joint_pos.data.resize(msg.data.size()-3);

  for (int i=3; i < msg.data.size(); i++){
    joint_pos.data[i-3] = msg.data[i];
  }
  ArmPosCommandPub_.publish(joint_pos);
  current_pose = base->getCurrentState();

}

void ArmController::JointStatePub(){
  ros::Rate loop_rate(20);
  vector<double> prevVel; for(int i = 0; i < 3; ++i) prevVel.push_back(0.0);
  for(int i = -1; i < 8; ++i) current_pose.push_back(0.0);
  while(ros::ok()){

    sensor_msgs::JointState jointStateMsg;
    double stepTime = 1 / 20;

    jointStateMsg.header.stamp = ros::Time::now();
    jointStateMsg.name.resize(3);
    jointStateMsg.name[0] ="base_jointx";
    jointStateMsg.name[1] ="base_jointy";
    jointStateMsg.name[2] ="base_jointz";

    vector<double> actual_state = base -> getCurrentState();
    jointStateMsg.position = actual_state;
    jointStateMsg.velocity = computeDerivative(jointStateMsg.position, current_pose, stepTime);
    jointStateMsg.effort = computeDerivative(jointStateMsg.velocity, prevVel, stepTime);

    JointStatesPub_ = nh_.advertise<sensor_msgs::JointState>("/base/joint_states", 1);
    JointStatesPub_.publish(jointStateMsg);
    ros::spinOnce();
    loop_rate.sleep();
  }
}

vector<double> ArmController::computeDerivative(vector<int> v1, vector<int> v2, double timeStep) {
    vector<double> der;
    for(int i = 0; i < v1.size(); ++i)
        der.push_back((v2.at(i) - v1.at(i)) / timeStep);
    return der;
}

vector<double> ArmController::computeDerivative(vector<double> v1, vector<double> v2, double timeStep) {
    vector<double> der;
    for(int i = 0; i < v1.size(); ++i)
        der.push_back((v2.at(i) - v1.at(i)) / timeStep);
    return der;
}

int main(int argc, char** argv){
  ros::init(argc, argv, "ArmController");
  ArmController arm_controller = ArmController();
  ros::Subscriber PosCommandSub_ = arm_controller.nh_.subscribe("/real/robotino/joint_control/move", 1, &ArmController::PosCommandSub_cb, &arm_controller);
  //ros::Publisher cmdVelPub_ = arm_controller.nh_.advertise<geometry_msgs::Twist>("cmd_rotatory", 1, &arm_controller);
  ros::Publisher ArmPosCommandPub_ = arm_controller.nh_.advertise<std_msgs::Float64MultiArray>("joint_group_position_controller/command", 1, &arm_controller);
  ros::Publisher JointStatesPub_ = arm_controller.nh_.advertise<sensor_msgs::JointState>("/base2/joint_states", 1, &arm_controller);
  arm_controller.JointStatePub();
  ros::spin();
  return 0;
}

