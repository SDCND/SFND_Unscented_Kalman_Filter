#include "ukf.h"
#include "Eigen/Dense"

using Eigen::MatrixXd;
using Eigen::VectorXd;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  //std_a_ = 30;
  std_a_ = 2.;

  // Process noise standard deviation yaw acceleration in rad/s^2
  //std_yawdd_ = 30;
  std_yawdd_ = 2.;
  
  /**
   * DO NOT MODIFY measurement noise values below.
   * These are provided by the sensor manufacturer.
   */

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  
  /**
   * End DO NOT MODIFY section for measurement noise values 
   */
  
  /**
   * TODO: Complete the initialization. See ukf.h for other member properties.
   * Hint: one or more values initialized above might be wildly off...
   */

  is_initialized_ = false;
  previous_timestamp_ = 0;

  //set state dimension
  n_x = 5;

  //set augmented dimension
  n_aug = 7;

  //define spreading parameter
  lambda = 3 - n_x;

  //covariance matrix
  P_ << 0.5, 0, 0, 0, 0,
	  0, 0.5, 0, 0, 0,
	  0, 0, 0.5, 0, 0,
	  0, 0, 0, 0.2, 0,
	  0, 0, 0, 0, 0.2;

  //measurement matrix
  H_ = MatrixXd(2, 5);
  H_ << 1, 0, 0, 0, 0,
	  0, 1, 0, 0, 0;

  //covariance matrix
  R_ = MatrixXd(2, 2);
  R_ << std_laspx_ * std_laspx_, 0,
	  0, std_laspy_*std_laspy_;

  //state vectors
  radar_z = VectorXd(3);
  lidar_z = VectorXd(2);
}

UKF::~UKF() {}

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Make sure you switch between lidar and radar
   * measurements.
   */
	if (!is_initialized_)
	{
		is_initialized_ = true;

		if (meas_package.sensor_type_ == MeasurementPackage::LASER)
		{
			x_ << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1],	0, 0, 0;
			previous_timestamp_ = meas_package.timestamp_;
		}
		else if (meas_package.sensor_type_ == MeasurementPackage::RADAR) 
		{
			double rho = meas_package.raw_measurements_[0];
			double phi = meas_package.raw_measurements_[1];
			double rho_dot = meas_package.raw_measurements_[2];

			x_ << rho * cos(phi), rho*sin(phi),	0.4*rho_dot, phi, 0;
			previous_timestamp_ = meas_package.timestamp_;
		}
	}

	delta_t = (meas_package.timestamp_ - previous_timestamp_) / 1000000.0;
	previous_timestamp_ = meas_package.timestamp_;

	Prediction(delta_t);

	if (meas_package.sensor_type_ == MeasurementPackage::LASER)
		UpdateLidar(meas_package);
	else
		UpdateRadar(meas_package);
}

void UKF::Prediction(double delta_t) {
  /**
   * TODO: Complete this function! Estimate the object's location. 
   * Modify the state vector, x_. Predict sigma points, the state, 
   * and the state covariance matrix.
   */

    //sigma points
	Eigen::MatrixXd Xsig_aug = MatrixXd(7, 15);
	AugmentedSigmaPoints(&Xsig_aug);

	MatrixXd Xsig_pred = MatrixXd(5, 15);
	SigmaPointPrediction(Xsig_aug, delta_t, &Xsig_pred);

	//predict
	VectorXd x_pred = VectorXd(5);
	MatrixXd P_pred = MatrixXd(5, 5);
	PredictMeanAndCovariance(Xsig_pred, &x_pred, &P_pred);

	//update
	x_ = x_pred;
	P_ = P_pred;
}

void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use lidar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the lidar NIS, if desired.
   */

	lidar_z << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1];

	Eigen::VectorXd z_pred = H_ * x_;
	Eigen::VectorXd y = lidar_z - z_pred;
	Eigen::MatrixXd Ht = H_.transpose();

	Eigen::MatrixXd S = H_ * P_ * Ht + R_;
	Eigen::MatrixXd Si = S.inverse();

	Eigen::MatrixXd PHt = P_ * Ht;
	Eigen::MatrixXd K = PHt * Si;

	x_ = x_ + K * y;

	//angle normalization
	if (x_(3) > M_PI)
		x_(3) -= 2.*M_PI;

	if (x_(3) < -M_PI)
		x_(3) += 2.*M_PI;

	size_t x_size = x_.size();
	Eigen::MatrixXd I = Eigen::MatrixXd::Identity(x_size, x_size);

	P_ = (I - K * H_) * P_;
}

void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use radar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the radar NIS, if desired.
   */

	Eigen::MatrixXd Xsig_aug = MatrixXd(7, 15);
	AugmentedSigmaPoints(&Xsig_aug);

	Eigen::MatrixXd Xsig_pred = MatrixXd(5, 15);
	SigmaPointPrediction(Xsig_aug, delta_t, &Xsig_pred);

	Eigen::MatrixXd Zsig = MatrixXd(3, 15);
	Eigen::VectorXd z_out = VectorXd(3);
	Eigen::MatrixXd S_out = MatrixXd(3, 3);

	radar_z << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1], meas_package.raw_measurements_[2];

	PredictRadarMeasurement(Xsig_pred, &Zsig, &z_out, &S_out);
	UpdateState(Xsig_pred, Zsig, z_out, S_out);
}

void UKF::GenerateSigmaPoints(Eigen::MatrixXd* Xsig_out) {

	int n_x = 5;

	//create sigma point matrix
	MatrixXd Xsig = MatrixXd(n_x, 2 * n_x + 1);

	//create square root of P
	MatrixXd A = P_.llt().matrixL();

	//set first column of sigma point matrix
	Xsig.col(0) = x_;

	//set remaining sigma points
	for (int i = 0; i < n_x; i++)
	{
		Xsig.col(i + 1) = x_ + sqrt(lambda + n_x) * A.col(i);
		Xsig.col(i + 1 + n_x) = x_ - sqrt(lambda + n_x) * A.col(i);
	}

	*Xsig_out = Xsig;
}


void UKF::AugmentedSigmaPoints(Eigen::MatrixXd* Xsig_out) {

	//create augmented mean vector
	VectorXd x_aug = VectorXd(n_aug);

	//create augmented state covariance
	MatrixXd P_aug = MatrixXd(n_aug, n_aug);

	//create sigma point matrix
	MatrixXd Xsig_aug = MatrixXd(n_aug, 2 * n_aug + 1);

	//create augmented mean state
	x_aug.head(5) = x_;
	x_aug(5) = 0;
	x_aug(6) = 0;

	// create augmented covariance matrix
	P_aug.fill(0.0);
	P_aug.topLeftCorner(5, 5) = P_;
	P_aug(5, 5) = std_a_ * std_a_;
	P_aug(6, 6) = std_yawdd_ * std_yawdd_;

	//create square root matrix
	MatrixXd L = P_aug.llt().matrixL();

	//create augmented sigma points
	Xsig_aug.col(0) = x_aug;

	for (int i = 0; i < n_aug; i++)
	{
		Xsig_aug.col(i + 1) = x_aug + sqrt(lambda + n_aug) * L.col(i);
		Xsig_aug.col(i + 1 + n_aug) = x_aug - sqrt(lambda + n_aug) * L.col(i);
	}

	*Xsig_out = Xsig_aug;
}


void UKF::SigmaPointPrediction(const Eigen::MatrixXd &Xsig_aug, const double &delta_t, Eigen::MatrixXd* Xsig_out) {

	//create matrix with predicted sigma points as columns
	MatrixXd Xsig_pred = MatrixXd(n_x, 2 * n_aug + 1);

	//predict sigma points
	for (int i = 0; i < 2 * n_aug + 1; i++) 
	{
		//extract values for better readability
		double p_x = Xsig_aug(0, i);
		double p_y = Xsig_aug(1, i);
		double v = Xsig_aug(2, i);
		double yaw = Xsig_aug(3, i);
		double yawd = Xsig_aug(4, i);
		double nu_a = Xsig_aug(5, i);
		double nu_yawdd = Xsig_aug(6, i);

		//predicted state values
		double px_p, py_p;

		//avoid division by zero
		if (fabs(yawd) > 0.001) 
		{
			px_p = p_x + v / yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
			py_p = p_y + v / yawd * (cos(yaw) - cos(yaw + yawd * delta_t));
		}
		else 
		{
			px_p = p_x + v * delta_t * cos(yaw);
			py_p = p_y + v * delta_t * sin(yaw);
		}

		double v_p = v;
		double yaw_p = yaw + yawd * delta_t;
		double yawd_p = yawd;

		//add noise
		px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
		py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
		v_p = v_p + nu_a * delta_t;

		yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
		yawd_p = yawd_p + nu_yawdd * delta_t;

		//write predicted sigma point into right column
		Xsig_pred(0, i) = px_p;
		Xsig_pred(1, i) = py_p;
		Xsig_pred(2, i) = v_p;
		Xsig_pred(3, i) = yaw_p;
		Xsig_pred(4, i) = yawd_p;
	}

	*Xsig_out = Xsig_pred;
}


void UKF::PredictMeanAndCovariance(const Eigen::MatrixXd &Xsig_pred, Eigen::VectorXd* x_out, Eigen::MatrixXd* P_out) {

	//create vector for weights
	VectorXd weights = VectorXd(2 * n_aug + 1);

	//create vector for predicted state
	VectorXd x = VectorXd(n_x);

	//create covariance matrix for prediction
	MatrixXd P = MatrixXd(n_x, n_x);

	//set weights
	double weight_0 = lambda / (lambda + n_aug);
	weights(0) = weight_0;

	for (int i = 1; i < 2 * n_aug + 1; i++)
	{
		double weight = 0.5 / (n_aug + lambda);
		weights(i) = weight;
	}

	//predicted state mean
	x.fill(0.0);

	for (int i = 0; i < 2 * n_aug + 1; i++)
	{
		x = x + weights(i)*Xsig_pred.col(i);
	}

	//predicted state covariance matrix
	P.fill(0.0);

	for (int i = 0; i < 2 * n_aug + 1; i++)
	{
		//state difference
		VectorXd x_diff = Xsig_pred.col(i) - x;

		//angle normalization
		if (x_diff(3) > M_PI)
			x_diff(3) -= 2.0 * M_PI;

		if (x_diff(3) < -M_PI)
			x_diff(3) += 2.0 * M_PI;

		P = P + weights(i) * x_diff * x_diff.transpose();
	}

	*x_out = x;
	*P_out = P;
}


void UKF::PredictRadarMeasurement(const Eigen::MatrixXd &Xsig_pred, Eigen::MatrixXd* zsig_out, Eigen::VectorXd* z_out, Eigen::MatrixXd* S_out) {

	//set measurement dimension, radar can measure r, phi, and r_dot
	int n_z = 3;

	//set vector for weights
	VectorXd weights = VectorXd(2 * n_aug + 1);

	double weight_0 = lambda / (lambda + n_aug);
	double weight = 0.5 / (lambda + n_aug);

	weights(0) = weight_0;

	for (int i = 1; i < 2 * n_aug + 1; ++i)
		weights(i) = weight;

	//create matrix for sigma points in measurement space
	MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug + 1);

	//create vector for mean predicted measurement
	VectorXd z_pred = VectorXd(n_z);

	//create matrix for predicted measurement covariance
	MatrixXd S = MatrixXd(n_z, n_z);

	//transform sigma points into measurement space
	for (int i = 0; i < 2 * n_aug + 1; i++)  //2n+1 simga points
	{
		double p_x = Xsig_pred(0, i);
		double p_y = Xsig_pred(1, i);
		double v = Xsig_pred(2, i);
		double yaw = Xsig_pred(3, i);

		double v1 = cos(yaw) * v;
		double v2 = sin(yaw) * v;

		//mesurement model
		Zsig(0, i) = sqrt(p_x * p_x + p_y * p_y);							//r
		Zsig(1, i) = atan2(p_y, p_x);										//phi
		Zsig(2, i) = (p_x * v1 + p_y * v2) / sqrt(p_x * p_x + p_y * p_y);	//r_dot
	}

	//mean predicted measurement
	z_pred.fill(0.0);

	for (int i = 0; i < 2 * n_aug + 1; i++)
		z_pred = z_pred + weights(i) * Zsig.col(i);

	//measurement covariance matrix S
	S.fill(0.0);

	for (int i = 0; i < 2 * n_aug + 1; i++)
	{
		VectorXd z_diff = Zsig.col(i) - z_pred;

		//angle normalization
		if (z_diff(1) > M_PI)
			z_diff(1) -= 2.0 * M_PI;

		if (z_diff(1) < -M_PI)
			z_diff(1) += 2.0 * M_PI;

		S = S + weights(i)*z_diff*z_diff.transpose();
	}

	//add measurement noise covariance matrix
	MatrixXd R = MatrixXd(n_z, n_z);

	R << std_radr_ * std_radr_, 0, 0,
		0, std_radphi_*std_radphi_, 0,
		0, 0, std_radrd_*std_radrd_;

	S = S + R;

	*zsig_out = Zsig;
	*z_out = z_pred;
	*S_out = S;
}


void UKF::UpdateState(const Eigen::MatrixXd &Xsig_pred, const Eigen::MatrixXd &Zsig, const Eigen::VectorXd &z_pred, const Eigen::MatrixXd &S) {

	//set measurement dimension, radar can measure r, phi, and r_dot
	int n_z = 3;

	//set vector for weights
	VectorXd weights = VectorXd(2 * n_aug + 1);

	double weight_0 = lambda / (lambda + n_aug);
	double weight = 0.5 / (lambda + n_aug);

	weights(0) = weight_0;

	for (int i = 1; i < 2 * n_aug + 1; i++)
		weights(i) = weight;

	//create matrix for cross correlation Tc
	MatrixXd Tc = MatrixXd(n_x, n_z);

	//calculate cross correlation matrix
	Tc.fill(0.0);

	for (int i = 0; i < 2 * n_aug + 1; i++)
	{
		VectorXd z_diff = Zsig.col(i) - z_pred;

		//angle normalization
		if (z_diff(1) > M_PI)
			z_diff(1) -= 2.0 * M_PI;

		if (z_diff(1) < -M_PI)
			z_diff(1) += 2.0 * M_PI;

		//state difference
		VectorXd x_diff = Xsig_pred.col(i) - x_;

		//angle normalization
		if (x_diff(3) > M_PI)
			x_diff(3) -= 2.0 * M_PI;

		if (x_diff(3) < -M_PI)
			x_diff(3) += 2.0 * M_PI;

		Tc = Tc + weights(i) * x_diff * z_diff.transpose();
	}

	//Kalman gain K;
	MatrixXd K = Tc * S.inverse();

	//residual
	VectorXd z_diff = radar_z - z_pred;

	//angle normalization
	if (z_diff(1) > M_PI)
		z_diff(1) -= 2.0 * M_PI;

	if (z_diff(1) < -M_PI)
		z_diff(1) += 2.0 * M_PI;

	//update state mean and covariance matrix
	x_ = x_ + K * z_diff;
	P_ = P_ - K * S * K.transpose();
}