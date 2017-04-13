#include <iostream>
#include "fusion_ekf.h"
#include "utilities.h"
#include "Eigen/Dense"


FusionEKF::FusionEKF() {

  is_initialized_ = false;
  time_us_ = 0;

  //state covariance matrix
  ekf_.p_ = Eigen::MatrixXd(4, 4);
  ekf_.p_ << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1000, 0,
            0, 0, 0, 1000;

  //the initial transition matrix F_
  ekf_.f_ = Eigen::MatrixXd(4, 4);
  ekf_.f_ << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;

  //measurement matrix for the basic Kalman fiter
  ekf_.h_ = Eigen::MatrixXd(2, 4);
  ekf_.h_ << 1, 0, 0, 0,
            0, 1, 0, 0;

  //measurement covariance matrices
  r_lidar_ = Eigen::MatrixXd(2, 2);
  r_lidar_ << 0.0225, 0,
              0, 0.0225;

  r_radar_ = Eigen::MatrixXd(3, 3);
  r_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  //set the acceleration noise components
  noise_ax_ = 9;
  noise_ay_ = 9;

}

FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {

  // Initialization
  if (!is_initialized_) {

    ekf_.x_ = Eigen::VectorXd(4);

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      Eigen::VectorXd x_polar(3);
      x_polar << measurement_pack.raw_measurements_[0],
          measurement_pack.raw_measurements_[1],
          measurement_pack.raw_measurements_[2];

      // Convert positions and velocities from polar to cartesian coordinates.
      Utilities utilities;
      ekf_.x_ = utilities.Polar2Cartesian(x_polar);
    } else {
      ekf_.x_ << measurement_pack.raw_measurements_[0],
          measurement_pack.raw_measurements_[1], 0, 0;
    }

    time_us_ = measurement_pack.timestamp_;
    is_initialized_ = true;
    return;
  }

  // Compute the time elapsed between the current and previous measurements
  double dt = (measurement_pack.timestamp_ - time_us_)
              / 1000000.0;  // In second

  // Update the state transition matrix f according to the new elapsed time.
  ekf_.f_(0, 2) = dt;
  ekf_.f_(1, 3) = dt;

  // Update the process noise covariance matrix.
  double dt_2 = dt*dt;
  double dt_3 = dt_2*dt;
  double dt_4 = dt_3*dt;

  ekf_.q_ = Eigen::MatrixXd(4, 4);

  ekf_.q_ << dt_4/4*noise_ax_, 0, dt_3/2*noise_ax_, 0,
            0, dt_4/4*noise_ay_, 0, dt_3/2*noise_ay_,
            dt_3/2*noise_ax_, 0, dt_2*noise_ax_, 0,
            0, dt_3/2*noise_ay_, 0, dt_2*noise_ay_;

  // Prediction and Measurement updating
  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Skip updating when near the origin.
    double rho = ekf_.x_[0]*ekf_.x_[0] + ekf_.x_[1]*ekf_.x_[1];
    if (rho > 1e-12) {
      ekf_.EKF(measurement_pack.raw_measurements_, r_radar_);
    } else {
      return;
    }
  } else {
    ekf_.KF(measurement_pack.raw_measurements_, r_lidar_);
  }

  // The latest time update should be put here since the update may
  // be skipped, e.g. for the RADAR measurement.
  time_us_ = measurement_pack.timestamp_;

}
