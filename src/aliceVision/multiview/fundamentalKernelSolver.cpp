// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <aliceVision/numeric/polynomial.hpp>
#include <aliceVision/multiview/fundamentalKernelSolver.hpp>

namespace aliceVision {
namespace fundamental {
namespace kernel {

using namespace std;
void SevenPointSolver::Solve(const Mat &x1, const Mat &x2, vector<Mat3> *F) {
  assert(2 == x1.rows());
  assert(7 <= x1.cols());
  assert(x1.rows() == x2.rows());
  assert(x1.cols() == x2.cols());

  Vec9 f1, f2;
  if (x1.cols() == 7) {
    // Set up the homogeneous system Af = 0 from the equations x'T*F*x = 0.
    typedef Eigen::Matrix<double, 9, 9> Mat9;
    // In the minimal solution use fixed sized matrix to let Eigen and the
    //  compiler doing the maximum of optimization.
    Mat9 A = Mat::Zero(9,9);
    EncodeEpipolarEquation(x1, x2, &A);
//    Eigen::FullPivLU<Mat9> luA(A);
//    ALICEVISION_LOG_DEBUG("\n rank(A) = " << luA.rank()); 
//    Eigen::JacobiSVD<Mat9> svdA(A);
//    cout << "Its singular values are:" << endl << svdA.singularValues() << endl;
    // Find the two F matrices in the nullspace of A.
    Nullspace2(&A, &f1, &f2);
    //@fixme here there is a potential error, we should check that the size of
    // null(A) is 2. Otherwise we have a family of possible solutions for the
    // fundamental matrix (ie infinite solution). This happens, e.g., when matching
    // the image against itself or in other degenerate configurations of the camera,
    // such as pure rotation or correspondences all on the same plane (cf HZ pg296 table 11.1)
    // This is not critical for just matching images with geometric validation, 
    // it becomes an issue if the estimated F has to be used for retrieving the 
    // motion of the camera.
  }
  else
  {
    // Set up the homogeneous system Af = 0 from the equations x'T*F*x = 0.
    Mat A(x1.cols(), 9);
    EncodeEpipolarEquation(x1, x2, &A);
    // Find the two F matrices in the nullspace of A.
    Nullspace2(&A, &f1, &f2);
  }

  Mat3 F1 = Map<RMat3>(f1.data());
  Mat3 F2 = Map<RMat3>(f2.data());

  // Then, use the condition det(F) = 0 to determine F. In other words, solve
  // det(F1 + a*F2) = 0 for a.
  double a = F1(0, 0), j = F2(0, 0),
          b = F1(0, 1), k = F2(0, 1),
          c = F1(0, 2), l = F2(0, 2),
          d = F1(1, 0), m = F2(1, 0),
          e = F1(1, 1), n = F2(1, 1),
          f = F1(1, 2), o = F2(1, 2),
          g = F1(2, 0), p = F2(2, 0),
          h = F1(2, 1), q = F2(2, 1),
          i = F1(2, 2), r = F2(2, 2);

  // Run fundamental_7point_coeffs.py to get the below coefficients.
  // The coefficients are in ascending powers of alpha, i.e. P[N]*x^N.
  double P[4] = {
    a*e*i + b*f*g + c*d*h - a*f*h - b*d*i - c*e*g,
    a*e*r + a*i*n + b*f*p + b*g*o + c*d*q + c*h*m + d*h*l + e*i*j + f*g*k -
    a*f*q - a*h*o - b*d*r - b*i*m - c*e*p - c*g*n - d*i*k - e*g*l - f*h*j,
    a*n*r + b*o*p + c*m*q + d*l*q + e*j*r + f*k*p + g*k*o + h*l*m + i*j*n -
    a*o*q - b*m*r - c*n*p - d*k*r - e*l*p - f*j*q - g*l*n - h*j*o - i*k*m,
    j*n*r + k*o*p + l*m*q - j*o*q - k*m*r - l*n*p};

  // Solve for the roots of P[3]*x^3 + P[2]*x^2 + P[1]*x + P[0] = 0.
  double roots[3];
  int num_roots = SolveCubicPolynomial(P, roots);

  // Build the fundamental matrix for each solution.
  for (int kk = 0; kk < num_roots; ++kk)  {
    F->push_back(F1 + roots[kk] * F2);
  }
}

void EightPointSolver::Solve(const Mat &x1, const Mat &x2, vector<Mat3> *Fs, const vector<double> *weights) 
{
  assert(2 == x1.rows());
  assert(8 <= x1.cols());
  assert(x1.rows() == x2.rows());
  assert(x1.cols() == x2.cols());

  Vec9 f;
  if (x1.cols() == 8) 
  {
    typedef Eigen::Matrix<double, 9, 9> Mat9;
    // In the minimal solution use fixed sized matrix to let Eigen and the
    //  compiler doing the maximum of optimization.
    Mat9 A = Mat::Zero(9,9);
    EncodeEpipolarEquation(x1, x2, &A, weights);
    Nullspace(&A, &f);
  }
  else  
  {
    MatX9 A(x1.cols(), 9);
    EncodeEpipolarEquation(x1, x2, &A, weights);
    Nullspace(&A, &f);
  }

  Mat3 F = Map<RMat3>(f.data());

  // Force the fundamental property if the A matrix has full rank.
  // HZ 11.1.1 pag.280
  if (x1.cols() > 8) 
  {
    // Force fundamental matrix to have rank 2
    Eigen::JacobiSVD<Mat3> USV(F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Vec3 d = USV.singularValues();
    d[2] = 0.0;
    F = USV.matrixU() * d.asDiagonal() * USV.matrixV().transpose();
  }
  Fs->push_back(F);
}

}  // namespace kernel
}  // namespace fundamental
}  // namespace aliceVision
