/*!
  \file        fake_face_detector_pplp.cpp
  \author      Arnaud Ramey <arnaud.a.ramey@gmail.com>
                -- Robotics Lab, University Carlos III of Madrid
  \date        2012/10/12

________________________________________________________________________________

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

\todo Description of the file

\section Parameters
  - \b "people_heads"
        [string] (default: "0,0,0")
        The position of the heads.

  - \b "x_oscill_ampl", "y_oscill_ampl", "z_oscill_ampl"
        [double] (default: 1)
        The oscillation amplitude for the people heads.

  - \b "x_oscill_speed", "y_oscill_speed", "z_oscill_speed"
        [double] (default: 0)
        The oscillation angular speed for the people heads.

  - \b "gaussian_error_sigma"
        [double] (default: 1)
        The guaussian error amplitude for the people heads.

  - \b target_frame
        [string] (default:"/odom")
        in which TF frame we want the found faces.

\section Publications
  - \b "~ppl"
        [people_msgs::People]
        The wanted people heads

 */
#include <ros/ros.h>
#include <tf/tf.h>
#include <people_msgs/People.h>

#include "vision_utils/timer.h"
#include "vision_utils/foo_point.h"

#include "vision_utils/string_split.h"

typedef vision_utils::FooPoint3f Point3;

int main(int argc, char** argv) {
  ros::init(argc, argv, "fake_face_detector_pplp");
  ros::NodeHandle nh_public, nh_private("~");
  std::string people_heads_str = "0,0,0";
  double gaussian_error_sigma = 1;
  double x_oscill_ampl = 0, y_oscill_ampl = 0, z_oscill_ampl = 0;
  double x_oscill_speed = 1, y_oscill_speed = 1, z_oscill_speed = 1;
  std::string frame_id = "/odom";

  // get params
  nh_private.param("people_heads", people_heads_str, people_heads_str);
  nh_private.param("x_oscill_ampl", x_oscill_ampl, x_oscill_ampl);
  nh_private.param("x_oscill_ampl", x_oscill_ampl, x_oscill_ampl);
  nh_private.param("y_oscill_ampl", y_oscill_ampl, y_oscill_ampl);
  nh_private.param("z_oscill_ampl", z_oscill_ampl, z_oscill_ampl);
  nh_private.param("x_oscill_speed", x_oscill_speed, x_oscill_speed);
  nh_private.param("y_oscill_speed", y_oscill_speed, y_oscill_speed);
  nh_private.param("z_oscill_speed", z_oscill_speed, z_oscill_speed);
  nh_private.param("frame_id", frame_id, frame_id);
  nh_private.param("gaussian_error_sigma",
                   gaussian_error_sigma, gaussian_error_sigma);

  // parse the params
  std::vector<std::string> people_head_str_list;
  std::vector<Point3> people_heads;
  vision_utils::StringSplit(people_heads_str, ";", &people_head_str_list);
  for (unsigned int face_idx = 0; face_idx < people_head_str_list.size(); ++face_idx) {
    std::vector<double> head_coordinates;
    vision_utils::StringSplit_<double>
        (people_head_str_list[face_idx], ",", &head_coordinates);
    if (head_coordinates.size() != 3) {
      ROS_WARN("The string '%s' is not a valid 3D point! Skipping it.",
               people_head_str_list[face_idx].c_str());
      continue;
    }
    people_heads.push_back(Point3(head_coordinates[0],
                           head_coordinates[1],
        head_coordinates[2]));
  } // end loop face_idx

  unsigned int n_people = people_heads.size();
  ROS_INFO("fake_face_detector_pplp: emitting %i faces (%s), gaussian_error_sigma:%f",
           n_people,
           vision_utils::accessible_to_string(people_heads).c_str(),
           gaussian_error_sigma);

  // build message for the first time
  people_msgs::People ppl;
  ppl.header.frame_id = frame_id;
  // the stamp will be set in the update loop
  ppl.people.clear();
  ppl.people.reserve(n_people);
  for (unsigned int face_idx = 0; face_idx < n_people; ++face_idx) {
    people_msgs::Person people_pose;
    vision_utils::set_method(people_pose, "fake_face_detector_pplp");
    // the header will be set in the update loop
    // the position and orientation will be set in the update loop
    people_pose.name = vision_utils::cast_to_string(face_idx);
    people_pose.reliability = 1;
    ppl.people.push_back(people_pose);
  } // end loop face_idx

  ros::Publisher ppl_pub = nh_private.advertise
      <people_msgs::People>("ppl", 1);

  // emit it periodically
  ros::Rate rate(15);
  vision_utils::Timer timer_start;
  while (ros::ok()) {
    double elapsed_time = timer_start.getTimeSeconds();
    ros::Time stamp = ros::Time::now();
    // change message content
    ppl.header.stamp = stamp;
    for (unsigned int face_idx = 0; face_idx < n_people; ++face_idx) {
      // copy header
      ppl.people[face_idx].header = ppl.header;
      geometry_msgs::Pose* position =
          &(ppl.people[face_idx].position);
      vision_utils::copy3(people_heads[face_idx],position->position);
      position->position.x +=
          x_oscill_ampl * cos(elapsed_time * x_oscill_speed) +
          vision_utils::rand_gaussian() * gaussian_error_sigma;
      position->position.y +=
          y_oscill_ampl * sin(elapsed_time * y_oscill_speed) +
          vision_utils::rand_gaussian() * gaussian_error_sigma;
      position->position.z +=
          z_oscill_ampl * cos(elapsed_time * z_oscill_speed) +
          vision_utils::rand_gaussian() * gaussian_error_sigma;
      position->orientation = tf::createQuaternionMsgFromYaw(0);

      //  ROS_WARN_THROTTLE(1, "head #%i: pose='%s'",
      //                    face_idx, vision_utils::print_pose(*position).c_str());
    } // end loop face_idx

    // publish message
    ppl_pub.publish(ppl);
    ros::spinOnce();
    rate.sleep();
  } // end while ros::ok
  return 0;
}
