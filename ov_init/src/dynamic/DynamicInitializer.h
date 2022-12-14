/*
 * SPL_VIO
 * Copyright (C) 2022-2023 Yanyu Zhang
 * Copyright (C) 2022-2023 Wei Ren
 */

#ifndef OV_INIT_DYNAMICINITIALIZER_H
#define OV_INIT_DYNAMICINITIALIZER_H

#include "ceres/Factor_GenericPrior.h"
#include "ceres/Factor_ImageReprojCalib.h"
#include "ceres/Factor_ImuCPIv1.h"
#include "ceres/State_JPLQuatLocal.h"
#include "init/InertialInitializerOptions.h"
#include "utils/helper.h"

#include "cpi/CpiV1.h"
#include "feat/FeatureHelper.h"
#include "feat/LineHelper.h"
#include "types/IMU.h"
#include "types/Landmark.h"
#include "types/Line_Landmark.h"
#include "utils/colors.h"
#include "utils/print.h"
#include "utils/quat_ops.h"
#include "utils/sensor_data.h"

namespace ov_init {

/**
 * @brief Initializer for a dynamic visual-inertial system.
 *
 * This implementation that will try to recover the initial conditions of the system.
 * Additionally, we will try to recover the covariance of the system.
 * To initialize with arbitrary motion:
 * 1. Preintegrate our system to get the relative rotation change (biases assumed known)
 * 2. Construct linear system with features to recover velocity (solve with |g| constraint)
 * 3. Perform a large MLE with all calibration and recover the covariance.
 *
 * Method is based on this work:
 * > Dong-Si, Tue-Cuong, and Anastasios I. Mourikis.
 * > "Estimator initialization in vision-aided inertial navigation with unknown camera-IMU calibration."
 * > 2012 IEEE/RSJ International Conference on Intelligent Robots and Systems. IEEE, 2012.
 *
 * - https://ieeexplore.ieee.org/abstract/document/6386235
 * - https://tdongsi.github.io/download/pubs/2011_VIO_Init_TR.pdf
 *
 */
class DynamicInitializer {
public:
  /**
   * @brief Default constructor
   * @param params_ Parameters loaded from either ROS or CMDLINE
   * @param db Feature tracker database with all features in it
   * @param imu_data_ Shared pointer to our IMU vector of historical information
   */
  explicit DynamicInitializer(const InertialInitializerOptions &params_, std::shared_ptr<ov_core::FeatureDatabase> db, std::shared_ptr<ov_core::LineDatabase> line_db, std::shared_ptr<std::vector<ov_core::ImuData>> imu_data_)
      : params(params_), _db(db), _line_db(line_db), imu_data(imu_data_) {}

  /**
   * @brief Try to get the initialized system
   *
   * @param[out] timestamp Timestamp we have initialized the state at (last imu state)
   * @param[out] covariance Calculated covariance of the returned state
   * @param[out] order Order of the covariance matrix
   * @param _imu Pointer to the "active" IMU state (q_GtoI, p_IinG, v_IinG, bg, ba)
   * @param _clones_IMU Map between imaging times and clone poses (q_GtoIi, p_IiinG)
   * @param _features_SLAM Our current set of SLAM features (3d positions)
   * @param _calib_IMUtoCAM Calibration poses for each camera (R_ItoC, p_IinC)
   * @param _cam_intrinsics Camera intrinsics
   * @return True if we have successfully initialized our system
   */
  bool initialize(double &timestamp, 
                  Eigen::MatrixXd &covariance, 
                  std::vector<std::shared_ptr<ov_type::Type>> &order, 
                  std::shared_ptr<ov_type::IMU> &_imu, 
                  std::map<double, std::shared_ptr<ov_type::PoseJPL>> &_clones_IMU, 
                  std::unordered_map<size_t, std::shared_ptr<ov_type::Landmark>> &_features_SLAM, 
                  std::unordered_map<size_t, std::shared_ptr<ov_type::Line_Landmark>> &_lines_SLAM, 
                  std::unordered_map<size_t, std::shared_ptr<ov_type::PoseJPL>> &_calib_IMUtoCAM, 
                  std::unordered_map<size_t, std::shared_ptr<ov_type::Vec>> &_cam_intrinsics);

private:
  /// Initialization parameters
  InertialInitializerOptions params;

  /// Feature tracker database with all features in it
  std::shared_ptr<ov_core::FeatureDatabase> _db;
  
  std::shared_ptr<ov_core::LineDatabase> _line_db;

  /// Our history of IMU messages (time, angular, linear)
  std::shared_ptr<std::vector<ov_core::ImuData>> imu_data;
};

} // namespace ov_init

#endif // OV_INIT_DYNAMICINITIALIZER_H
