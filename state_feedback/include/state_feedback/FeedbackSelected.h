/**
 * @brief Select necessary feedback header
 * @file FeedbackSelected.h
 * @author Murali VNV <muralivnv@gmail.com>
 */
/*
 * Copyright (c) 2017, muralivnv
 *
 * This file is part of the asl_gremlin_package and subject to the license terms
 * in the top-level LICENSE file of the asl_gremlin_pkg repository.
 * https://github.com/muralivnv/asl_gremlin_pkg/blob/master/LICENSE
 */
#ifndef _state_feedback_FEEDBACKSELECTED_H_
#define _state_feedback_FEEDBACKSELECTED_H_

#include <dynamic_reconfigure/server.h>
#include <state_feedback/feedbackSelectConfig.h>
#include <ros/ros.h>
#include <geometry_msgs/PointStamped.h>
#include <asl_gremlin_msgs/VehicleState.h>
#include <std_msgs/Float64.h>
#include <asl_gremlin_pkg/SubscribeTopic.h>
#include <utility_pkg/utilities.h>

#include <cmath>
#include <array>

enum Feedback{
    gps_compass,
    encoder_compass,
    pure_gps
};

using pose_type = asl_gremlin_msgs::VehicleState;

namespace state_feedback{

template<int NumOfFeedbacks>
class FeedbackSelected{

    pose_type* pose_;

    int feedback = Feedback::gps_compass;

    dynamic_reconfigure::Server<feedbackSelectConfig> dr_feedback_srv_;
    dynamic_reconfigure::Server<feedbackSelectConfig>::CallbackType fun_;

    ros::Publisher feedback_pub_;
    
    asl_gremlin_pkg::SubscribeTopic<geometry_msgs::PointStamped>* gps_pose_data_;
    asl_gremlin_pkg::SubscribeTopic<geometry_msgs::PointStamped>* enco_pose_data_;
    asl_gremlin_pkg::SubscribeTopic<std_msgs::Float64>* compass_data_;

    public:
        FeedbackSelected(ros::NodeHandle&);
        ~FeedbackSelected();

        void dynamic_reconfigure_feedback_callback(feedbackSelectConfig& , uint32_t);
        void get_gps_data();
        void get_enco_data();
        void publish();
};

template<int NumOfFeedback>
FeedbackSelected<NumOfFeedback>::FeedbackSelected(ros::NodeHandle& nh) : pose_(new pose_type[NumOfFeedback])
{                
    fun_ = boost::bind(&FeedbackSelected::dynamic_reconfigure_feedback_callback,
                        this, _1, _2);

    dr_feedback_srv_.setCallback(fun_);

    std::string feedback_topic, enco_pose_topic, gps_pose_topic;

    if(!nh.getParam("state_feedback/feedback_selected", feedback_topic))
    { feedback_topic = "state_feedback/selected_feedback"; }

    if(!nh.getParam("state_feedback/encoder/pose_topic", enco_pose_topic))
    { enco_pose_topic = "state_feedback/position_from_encoder"; }

    if(!nh.getParam("state_feedback/gps/pose_topic", gps_pose_topic))
    { gps_pose_topic = "state_feedback/position_from_gps"; }

    feedback_pub_ = nh.advertise<pose_type>(feedback_topic, 10);

    gps_pose_data_  = new asl_gremlin_pkg::SubscribeTopic<geometry_msgs::PointStamped>(nh, gps_pose_topic,10);
    enco_pose_data_ = new asl_gremlin_pkg::SubscribeTopic<geometry_msgs::PointStamped>(nh, enco_pose_topic,10);
    compass_data_   = new asl_gremlin_pkg::SubscribeTopic<std_msgs::Float64>
                      (nh,"mavros/global_position/compass_hdg" ,10);

    ros::spinOnce();
}

template<int NumOfFeedback>
FeedbackSelected<NumOfFeedback>::~FeedbackSelected()
{
    delete[] pose_;
    delete compass_data_;
    delete gps_pose_data_;
    delete enco_pose_data_;
}

template<int NumOfFeedback>
void FeedbackSelected<NumOfFeedback>::dynamic_reconfigure_feedback_callback(feedbackSelectConfig& config, uint32_t level)
{
    feedback = config.feedback;
    switch(feedback)
    {
        case(0):
            ROS_INFO("\033[0;33mUpdated\033[0;m:= {Feedback}-> \033[1;37m'GPS+Compass'\033[0;m");
            break;
        case(1):
            ROS_INFO("\033[0;33mUpdated\033[0;m:= {Feedback}-> \033[1;37m'Encoder+Compass'\033[0;m");
            break;
        case(2):
            ROS_INFO("\033[0;33mUpdated\033[0;m:= {Feedback}-> \033[1;37m'Pure GPS'\033[0;m");
            break;
    }
}

template<int NumOfFeedback>
void FeedbackSelected<NumOfFeedback>::get_gps_data()
{
    double theta_enu = utility_pkg::compass_angle_to_polar_angle((compass_data_->get_data())->data);
    
    pose_[0].pose.point  = (gps_pose_data_->get_data())->point;
    pose_[0].pose.header = (gps_pose_data_->get_data())->header;
    pose_[0].pose.header.frame_id = "local_ENU/GPS+Compass";
    pose_[0].heading     = theta_enu;

    pose_[1].pose.point.z = (gps_pose_data_->get_data())->point.z;
    pose_[1].heading = theta_enu;
    
    double theta_gps = std::atan2((gps_pose_data_->get_data())->point.y - pose_[2].pose.point.y,
                                    (gps_pose_data_->get_data())->point.x - pose_[2].pose.point.x);

    pose_[2].pose.point  = (gps_pose_data_->get_data())->point;
    pose_[2].pose.header = (gps_pose_data_->get_data())->header;
    pose_[2].pose.header.frame_id = "local_ENU/Pure-GPS";
    pose_[2].heading     = theta_gps * 180.0/M_PI;
}


template<int NumOfFeedback>
void FeedbackSelected<NumOfFeedback>::get_enco_data()
{
    pose_[1].pose.header = (enco_pose_data_->get_data())->header;
    pose_[1].pose.header.frame_id = "local_ENU/Encoder+Compass";
    pose_[1].pose.point.x = (enco_pose_data_->get_data())->point.x;
    pose_[1].pose.point.y = (enco_pose_data_->get_data())->point.y;
}

template<int NumOfFeedback>
void FeedbackSelected<NumOfFeedback>::publish()
{ feedback_pub_.publish(pose_[feedback]); }

} // end namespace { state_feedback}


#endif
