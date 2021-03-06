#ifndef UKF_H
#define UKF_H

#include "Eigen/Dense"
#include "measurement_package.h"

class UKF {
 public:
  /**
   * Constructor
   */
  UKF();

  /**
   * Destructor
   */
  virtual ~UKF();

  /**
   * ProcessMeasurement
   * @param meas_package The latest measurement data of either radar or laser
   */
  void ProcessMeasurement(MeasurementPackage meas_package);

  /**
   * Prediction Predicts sigma points, the state, and the state covariance
   * matrix
   * @param delta_t Time between k and k+1 in s
   */
  void Prediction(double delta_t);

  /**
   * Updates the state and the state covariance matrix using a laser measurement
   * @param meas_package The measurement at k+1
   */
  void UpdateLidar(MeasurementPackage meas_package);

  /**
   * Updates the state and the state covariance matrix using a radar measurement
   * @param meas_package The measurement at k+1
   */
  void UpdateRadar(MeasurementPackage meas_package);


  // initially set to false, set to true in first call of ProcessMeasurement
  bool is_initialized_;

  size_t previous_timestamp_;
  double delta_t;

  // if this is false, laser measurements will be ignored (except for init)
  bool use_laser_;

  // if this is false, radar measurements will be ignored (except for init)
  bool use_radar_;

  // state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
  Eigen::VectorXd x_;

  // state covariance matrix
  Eigen::MatrixXd P_;

  // predicted sigma points matrix
  Eigen::MatrixXd Xsig_pred_;

  Eigen::MatrixXd H_;
  Eigen::MatrixXd R_;
  Eigen::VectorXd radar_z;
  Eigen::VectorXd lidar_z;

  // time when the state is true, in us
  long long time_us_;

  // Process noise standard deviation longitudinal acceleration in m/s^2
  double std_a_;

  // Process noise standard deviation yaw acceleration in rad/s^2
  double std_yawdd_;

  // Laser measurement noise standard deviation position1 in m
  double std_laspx_;

  // Laser measurement noise standard deviation position2 in m
  double std_laspy_;

  // Radar measurement noise standard deviation radius in m
  double std_radr_;

  // Radar measurement noise standard deviation angle in rad
  double std_radphi_;

  // Radar measurement noise standard deviation radius change in m/s
  double std_radrd_ ;

  // Weights of sigma points
  Eigen::VectorXd weights_;

  // State dimension
  int n_x;

  // Augmented state dimension
  int n_aug;

  // Sigma point spreading parameter
  double lambda;

private:

	void GenerateSigmaPoints(Eigen::MatrixXd* Xsig_out);
	void AugmentedSigmaPoints(Eigen::MatrixXd* Xsig_out);
	void SigmaPointPrediction(const Eigen::MatrixXd &Xsig_aug, const double &delta_t, Eigen::MatrixXd* Xsig_out);
	void PredictMeanAndCovariance(const Eigen::MatrixXd &Xsig_pred, Eigen::VectorXd* x_out, Eigen::MatrixXd* P_out);
	void PredictRadarMeasurement(const Eigen::MatrixXd &Xsig_pred, Eigen::MatrixXd* zsig_out, Eigen::VectorXd* z_out, Eigen::MatrixXd* S_out);
	void UpdateState(const Eigen::MatrixXd &Xsig_pred, const Eigen::MatrixXd &Zsig, const Eigen::VectorXd &z_pred, const Eigen::MatrixXd &S);
};

#endif  // UKF_H