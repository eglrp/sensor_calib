#ifndef FISHEYEMODEL_H
#define FISHEYEMODEL_H

#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

class OmniModel
{
public:
	OmniModel(cv::Mat intrinsics, cv::Mat distortionCoeff, double mirror){ // intrinsics: 3 by 3 distortionCoeff: 4 by 1
		m_u0 = intrinsics.at<double>(0, 2);
		m_v0 = intrinsics.at<double>(1, 2);
		m_fu = intrinsics.at<double>(0, 0);
		m_fv = intrinsics.at<double>(1, 1);
		m_xi = mirror;
		m_k1 = distortionCoeff.at<double>(0, 0);
		m_k2 = distortionCoeff.at<double>(1, 0);
		m_p1 = distortionCoeff.at<double>(2, 0);
		m_p2 = distortionCoeff.at<double>(3, 0);
		m_fov_parameter = m_xi <= 1 ? m_xi : 1.0/m_xi;
	}

	OmniModel(cv::Mat intrinsics, cv::Mat distortionCoeff, double mirror, 
			double u0, double v0){ 
		m_u0 = u0;
		m_v0 = v0;
		m_fu = intrinsics.at<double>(0, 0);
		m_fv = intrinsics.at<double>(1, 1);
		m_xi = mirror;
		m_k1 = distortionCoeff.at<double>(0, 0);
		m_k2 = distortionCoeff.at<double>(1, 0);
		m_p1 = distortionCoeff.at<double>(2, 0);
		m_p2 = distortionCoeff.at<double>(3, 0);
		m_fov_parameter = m_xi <= 1 ? m_xi : 1.0/m_xi;
	}

	OmniModel(){}

	void setParameter(cv::Mat intrinsics, cv::Mat distortionCoeff, double mirror){ // intrinsics: 3 by 3 distortionCoeff: 4 by 1
		m_u0 = intrinsics.at<double>(0, 2);
		m_v0 = intrinsics.at<double>(1, 2);
		m_fu = intrinsics.at<double>(0, 0);
		m_fv = intrinsics.at<double>(1, 1);
		m_xi = mirror;
		m_k1 = distortionCoeff.at<double>(0, 0);
		m_k2 = distortionCoeff.at<double>(1, 0);
		m_p1 = distortionCoeff.at<double>(2, 0);
		m_p2 = distortionCoeff.at<double>(3, 0);
	}

	void distortion(const double mx_u, const double my_u, 
                double *dx_u, double *dy_u, 
                double *dxdmx, double *dydmx,
                double *dxdmy, double *dydmy)const;

	// template<typename T>
	// void distortion(const T mx_u, const T my_u, 
 //                T *dx_u, T *dy_u, 
 //                T *dxdmx, T *dydmx,
 //                T *dxdmy, T *dydmy) const;
	template<typename T>
	void distortion(const T mx_u, const T my_u, 
	                T *dx_u, T *dy_u, 
	                T *dxdmx, T *dydmx,
	                T *dxdmy, T *dydmy) const{
	  T mx2_u, my2_u, mxy_u, rho2_u, rad_dist_u;

	  mx2_u = mx_u * mx_u;
	  my2_u = my_u * my_u;
	  mxy_u = mx_u * my_u;
	  rho2_u = mx2_u + my2_u;
	  rad_dist_u = T(m_k1) * rho2_u + T(m_k2) * rho2_u * rho2_u;
	  *dx_u = mx_u * rad_dist_u + T(2) * T(m_p1) * mxy_u + T(m_p2) * (rho2_u + T(2) * mx2_u);
	  *dy_u = my_u * rad_dist_u + T(2) * T(m_p2) * mxy_u + T(m_p1) * (rho2_u + T(2) * my2_u);

	  *dxdmx = T(1) + rad_dist_u + T(m_k1) * T(2) * mx2_u + T(m_k2) * rho2_u * T(4) * mx2_u
	      + T(2) * T(m_p1) * my_u + T(6) * T(m_p2) * mx_u;
	  *dydmx = T(m_k1) * T(2) * mx_u * my_u + T(m_k2) * T(4) * rho2_u * mx_u * my_u
	      + T(m_p1) * T(2) * mx_u + T(2) * T(m_p2) * my_u;
	  *dxdmy = *dydmx;
	  *dydmy = T(1) + rad_dist_u + T(m_k1) * T(2) * my2_u + T(m_k2) * rho2_u * T(4) * my2_u
	      + T(6) * T(m_p1) * my_u + T(2) * T(m_p2) * mx_u;
	}

	void undistortGN(const double u_d, const double v_d, double *u,
                                     double *v) const;

	void undistortImage(cv::Mat distorted, cv::Mat undistorted);

	bool isUndistortedKeypointValid(const double rho2_d) const;

	bool keypointToEuclidean(const Eigen::Vector2d & keypoint,
    							   Eigen::Vector3d & outPoint) const;
	
	template<typename T>
	bool euclideanToKeypoint(const Eigen::Matrix<T, 3, 1> & p,
                     Eigen::Matrix<T, 2, 1> & outKeypoint) const{
	  T d = p.norm();

	  // Check if point will lead to a valid projection
	  if (p[2] <= -(T(m_fov_parameter) * d))
	    return false;

	  T rz = 1.0 / (p[2] + T(m_xi) * d);
	  outKeypoint[0] = p[0] * rz;
	  outKeypoint[1] = p[1] * rz;

	  //////////////////////////////
	  // need to be checked again
	  ////////////////////////////
	  Eigen::Matrix<T, 2, 2> F;
	  T hat_u_d, hat_v_d;
	  distortion(outKeypoint[0], outKeypoint[1], &hat_u_d, &hat_v_d, &F(0, 0), &F(1, 0), &F(0, 1),
	               &F(1, 1));
	  outKeypoint[0] = outKeypoint[0] + hat_u_d;
	  outKeypoint[1] = outKeypoint[1] + hat_v_d; 
	  //std::cout << "distort\n";
	  //SM_OUT(outKeypoint[0]);
	  //SM_OUT(outKeypoint[1]);

	  outKeypoint[0] = T(m_fu) * outKeypoint[0] + T(m_u0);
	  outKeypoint[1] = T(m_fv) * outKeypoint[1] + T(m_v0);
	  //std::cout << "project\n";
	  //SM_OUT(outKeypoint[0]);
	  //SM_OUT(outKeypoint[1]);

	  // Check if keypoint lies on the sensor
	  return true; //isValid(outKeypoint);
	}
	bool estimateTransformation(std::vector<cv::Point2f> Ms,
								std::vector<cv::Point3f> Ps,
    							Eigen::Matrix4d &  out_T_t_c);
// private:
	double m_u0, m_v0, m_fu, m_fv, m_xi; //camera paramter
	double m_k1, m_k2, m_p1, m_p2; // distortion parameter
	double m_fov_parameter; //PM: is = xi for xi=<1, = 1/xi for x>1. Used for determining valid projections. Better name?

};
#endif