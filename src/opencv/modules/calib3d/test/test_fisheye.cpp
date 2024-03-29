/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "test_precomp.hpp"
#include <opencv2/ts/cuda_test.hpp> // EXPECT_MAT_NEAR
#include "../src/fisheye.hpp"
#include "opencv2/videoio.hpp"

namespace opencv_test { namespace {

class fisheyeTest : public ::testing::Test {

protected:
    const static cv::Size imageSize;
    const static cv::Matx33d K;
    const static cv::Vec4d D;
    const static cv::Matx33d R;
    const static cv::Vec3d T;
    std::string datasets_repository_path;

    virtual void SetUp() {
        datasets_repository_path = combine(cvtest::TS::ptr()->get_data_path(), "cv/cameracalibration/fisheye");
    }

protected:
    std::string combine(const std::string& _item1, const std::string& _item2);
    static void merge4(const cv::Mat& tl, const cv::Mat& tr, const cv::Mat& bl, const cv::Mat& br, cv::Mat& merged);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  TESTS::

TEST_F(fisheyeTest, projectPoints)
{
    double cols = this->imageSize.width,
           rows = this->imageSize.height;

    const int N = 20;
    cv::Mat distorted0(1, N*N, CV_64FC2), undist1, undist2, distorted1, distorted2;
    undist2.create(distorted0.size(), CV_MAKETYPE(distorted0.depth(), 3));
    cv::Vec2d* pts = distorted0.ptr<cv::Vec2d>();

    cv::Vec2d c(this->K(0, 2), this->K(1, 2));
    for(int y = 0, k = 0; y < N; ++y)
        for(int x = 0; x < N; ++x)
        {
            cv::Vec2d point(x*cols/(N-1.f), y*rows/(N-1.f));
            pts[k++] = (point - c) * 0.85 + c;
        }

    cv::fisheye::undistortPoints(distorted0, undist1, this->K, this->D);

    cv::Vec2d* u1 = undist1.ptr<cv::Vec2d>();
    cv::Vec3d* u2 = undist2.ptr<cv::Vec3d>();
    for(int i = 0; i  < (int)distorted0.total(); ++i)
        u2[i] = cv::Vec3d(u1[i][0], u1[i][1], 1.0);

    cv::fisheye::distortPoints(undist1, distorted1, this->K, this->D);
    cv::fisheye::projectPoints(undist2, distorted2, cv::Vec3d::all(0), cv::Vec3d::all(0), this->K, this->D);

    EXPECT_MAT_NEAR(distorted0, distorted1, 1e-10);
    EXPECT_MAT_NEAR(distorted0, distorted2, 1e-10);
}

// we use it to reduce patch size for images in testdata
static void throwAwayHalf(Mat img)
{
    int whalf = img.cols / 2, hhalf = img.rows / 2;
    Rect tl(0, 0, whalf, hhalf), br(whalf, hhalf, whalf, hhalf);
    img(tl) = 0;
    img(br) = 0;
};

TEST_F(fisheyeTest, undistortImage)
{
    cv::Matx33d theK = this->K;
    cv::Mat theD = cv::Mat(this->D);
    std::string file = combine(datasets_repository_path, "/calib-3_stereo_from_JY/left/stereo_pair_014.jpg");
    cv::Matx33d newK = theK;
    cv::Mat distorted = cv::imread(file), undistorted;
    {
        newK(0, 0) = 100;
        newK(1, 1) = 100;
        cv::fisheye::undistortImage(distorted, undistorted, theK, theD, newK);
        std::string imageFilename = combine(datasets_repository_path, "new_f_100.png");
        cv::Mat correct = cv::imread(imageFilename);
        ASSERT_FALSE(correct.empty()) << "Correct image " << imageFilename.c_str() << " can not be read" << std::endl;

        throwAwayHalf(correct);
        throwAwayHalf(undistorted);

        EXPECT_MAT_NEAR(correct, undistorted, 1e-10);
    }
    {
        double balance = 1.0;
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify(theK, theD, distorted.size(), cv::noArray(), newK, balance);
        cv::fisheye::undistortImage(distorted, undistorted, theK, theD, newK);
        std::string imageFilename = combine(datasets_repository_path, "balance_1.0.png");
        cv::Mat correct = cv::imread(imageFilename);
        ASSERT_FALSE(correct.empty()) << "Correct image " << imageFilename.c_str() << " can not be read" << std::endl;

        throwAwayHalf(correct);
        throwAwayHalf(undistorted);

        EXPECT_MAT_NEAR(correct, undistorted, 1e-10);
    }

    {
        double balance = 0.0;
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify(theK, theD, distorted.size(), cv::noArray(), newK, balance);
        cv::fisheye::undistortImage(distorted, undistorted, theK, theD, newK);
        std::string imageFilename = combine(datasets_repository_path, "balance_0.0.png");
        cv::Mat correct = cv::imread(imageFilename);
        ASSERT_FALSE(correct.empty()) << "Correct image " << imageFilename.c_str() << " can not be read" << std::endl;

        throwAwayHalf(correct);
        throwAwayHalf(undistorted);

        EXPECT_MAT_NEAR(correct, undistorted, 1e-10);
    }
}

TEST_F(fisheyeTest, jacobians)
{
    int n = 10;
    cv::Mat X(1, n, CV_64FC3);
    cv::Mat om(3, 1, CV_64F), theT(3, 1, CV_64F);
    cv::Mat f(2, 1, CV_64F), c(2, 1, CV_64F);
    cv::Mat k(4, 1, CV_64F);
    double alpha;

    cv::RNG r;

    r.fill(X, cv::RNG::NORMAL, 2, 1);
    X = cv::abs(X) * 10;

    r.fill(om, cv::RNG::NORMAL, 0, 1);
    om = cv::abs(om);

    r.fill(theT, cv::RNG::NORMAL, 0, 1);
    theT = cv::abs(theT); theT.at<double>(2) = 4; theT *= 10;

    r.fill(f, cv::RNG::NORMAL, 0, 1);
    f = cv::abs(f) * 1000;

    r.fill(c, cv::RNG::NORMAL, 0, 1);
    c = cv::abs(c) * 1000;

    r.fill(k, cv::RNG::NORMAL, 0, 1);
    k*= 0.5;

    alpha = 0.01*r.gaussian(1);

    cv::Mat x1, x2, xpred;
    cv::Matx33d theK(f.at<double>(0), alpha * f.at<double>(0), c.at<double>(0),
                     0,            f.at<double>(1), c.at<double>(1),
                     0,            0,    1);

    cv::Mat jacobians;
    cv::fisheye::projectPoints(X, x1, om, theT, theK, k, alpha, jacobians);

    //test on T:
    cv::Mat dT(3, 1, CV_64FC1);
    r.fill(dT, cv::RNG::NORMAL, 0, 1);
    dT *= 1e-9*cv::norm(theT);
    cv::Mat T2 = theT + dT;
    cv::fisheye::projectPoints(X, x2, om, T2, theK, k, alpha, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.colRange(11,14) * dT).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);

    //test on om:
    cv::Mat dom(3, 1, CV_64FC1);
    r.fill(dom, cv::RNG::NORMAL, 0, 1);
    dom *= 1e-9*cv::norm(om);
    cv::Mat om2 = om + dom;
    cv::fisheye::projectPoints(X, x2, om2, theT, theK, k, alpha, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.colRange(8,11) * dom).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);

    //test on f:
    cv::Mat df(2, 1, CV_64FC1);
    r.fill(df, cv::RNG::NORMAL, 0, 1);
    df *= 1e-9*cv::norm(f);
    cv::Matx33d K2 = theK + cv::Matx33d(df.at<double>(0), df.at<double>(0) * alpha, 0, 0, df.at<double>(1), 0, 0, 0, 0);
    cv::fisheye::projectPoints(X, x2, om, theT, K2, k, alpha, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.colRange(0,2) * df).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);

    //test on c:
    cv::Mat dc(2, 1, CV_64FC1);
    r.fill(dc, cv::RNG::NORMAL, 0, 1);
    dc *= 1e-9*cv::norm(c);
    K2 = theK + cv::Matx33d(0, 0, dc.at<double>(0), 0, 0, dc.at<double>(1), 0, 0, 0);
    cv::fisheye::projectPoints(X, x2, om, theT, K2, k, alpha, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.colRange(2,4) * dc).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);

    //test on k:
    cv::Mat dk(4, 1, CV_64FC1);
    r.fill(dk, cv::RNG::NORMAL, 0, 1);
    dk *= 1e-9*cv::norm(k);
    cv::Mat k2 = k + dk;
    cv::fisheye::projectPoints(X, x2, om, theT, theK, k2, alpha, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.colRange(4,8) * dk).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);

    //test on alpha:
    cv::Mat dalpha(1, 1, CV_64FC1);
    r.fill(dalpha, cv::RNG::NORMAL, 0, 1);
    dalpha *= 1e-9*cv::norm(f);
    double alpha2 = alpha + dalpha.at<double>(0);
    K2 = theK + cv::Matx33d(0, f.at<double>(0) * dalpha.at<double>(0), 0, 0, 0, 0, 0, 0, 0);
    cv::fisheye::projectPoints(X, x2, om, theT, theK, k, alpha2, cv::noArray());
    xpred = x1 + cv::Mat(jacobians.col(14) * dalpha).reshape(2, 1);
    CV_Assert (cv::norm(x2 - xpred) < 1e-10);
}

TEST_F(fisheyeTest, Calibration)
{
    const int n_images = 34;

    std::vector<std::vector<cv::Point2d> > imagePoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");
    cv::FileStorage fs_left(combine(folder, "left.xml"), cv::FileStorage::READ);
    CV_Assert(fs_left.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_left[cv::format("image_%d", i )] >> imagePoints[i];
    fs_left.release();

    cv::FileStorage fs_object(combine(folder, "object.xml"), cv::FileStorage::READ);
    CV_Assert(fs_object.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_object[cv::format("image_%d", i )] >> objectPoints[i];
    fs_object.release();

    int flag = 0;
    flag |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flag |= cv::fisheye::CALIB_CHECK_COND;
    flag |= cv::fisheye::CALIB_FIX_SKEW;

    cv::Matx33d theK;
    cv::Vec4d theD;

    cv::fisheye::calibrate(objectPoints, imagePoints, imageSize, theK, theD,
                           cv::noArray(), cv::noArray(), flag, cv::TermCriteria(3, 20, 1e-6));

    EXPECT_MAT_NEAR(theK, this->K, 1e-10);
    EXPECT_MAT_NEAR(theD, this->D, 1e-10);
}

TEST_F(fisheyeTest, Homography)
{
    const int n_images = 1;

    std::vector<std::vector<cv::Point2d> > imagePoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");
    cv::FileStorage fs_left(combine(folder, "left.xml"), cv::FileStorage::READ);
    CV_Assert(fs_left.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_left[cv::format("image_%d", i )] >> imagePoints[i];
    fs_left.release();

    cv::FileStorage fs_object(combine(folder, "object.xml"), cv::FileStorage::READ);
    CV_Assert(fs_object.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_object[cv::format("image_%d", i )] >> objectPoints[i];
    fs_object.release();

    cv::internal::IntrinsicParams param;
    param.Init(cv::Vec2d(cv::max(imageSize.width, imageSize.height) / CV_PI, cv::max(imageSize.width, imageSize.height) / CV_PI),
               cv::Vec2d(imageSize.width  / 2.0 - 0.5, imageSize.height / 2.0 - 0.5));

    cv::Mat _imagePoints (imagePoints[0]);
    cv::Mat _objectPoints(objectPoints[0]);

    cv::Mat imagePointsNormalized = NormalizePixels(_imagePoints, param).reshape(1).t();
    _objectPoints = _objectPoints.reshape(1).t();
    cv::Mat objectPointsMean, covObjectPoints;

    int Np = imagePointsNormalized.cols;
    cv::calcCovarMatrix(_objectPoints, covObjectPoints, objectPointsMean, cv::COVAR_NORMAL | cv::COVAR_COLS);
    cv::SVD svd(covObjectPoints);
    cv::Mat theR(svd.vt);

    if (cv::norm(theR(cv::Rect(2, 0, 1, 2))) < 1e-6)
        theR = cv::Mat::eye(3,3, CV_64FC1);
    if (cv::determinant(theR) < 0)
        theR = -theR;

    cv::Mat theT = -theR * objectPointsMean;
    cv::Mat X_new = theR * _objectPoints + theT * cv::Mat::ones(1, Np, CV_64FC1);
    cv::Mat H = cv::internal::ComputeHomography(imagePointsNormalized, X_new.rowRange(0, 2));

    cv::Mat M = cv::Mat::ones(3, X_new.cols, CV_64FC1);
    X_new.rowRange(0, 2).copyTo(M.rowRange(0, 2));
    cv::Mat mrep = H * M;

    cv::divide(mrep, cv::Mat::ones(3,1, CV_64FC1) * mrep.row(2).clone(), mrep);

    cv::Mat merr = (mrep.rowRange(0, 2) - imagePointsNormalized).t();

    cv::Vec2d std_err;
    cv::meanStdDev(merr.reshape(2), cv::noArray(), std_err);
    std_err *= sqrt((double)merr.reshape(2).total() / (merr.reshape(2).total() - 1));

    cv::Vec2d correct_std_err(0.00516740156010384, 0.00644205331553901);
    EXPECT_MAT_NEAR(std_err, correct_std_err, 1e-12);
}

TEST_F(fisheyeTest, EstimateUncertainties)
{
    const int n_images = 34;

    std::vector<std::vector<cv::Point2d> > imagePoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");
    cv::FileStorage fs_left(combine(folder, "left.xml"), cv::FileStorage::READ);
    CV_Assert(fs_left.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_left[cv::format("image_%d", i )] >> imagePoints[i];
    fs_left.release();

    cv::FileStorage fs_object(combine(folder, "object.xml"), cv::FileStorage::READ);
    CV_Assert(fs_object.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_object[cv::format("image_%d", i )] >> objectPoints[i];
    fs_object.release();

    int flag = 0;
    flag |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flag |= cv::fisheye::CALIB_CHECK_COND;
    flag |= cv::fisheye::CALIB_FIX_SKEW;

    cv::Matx33d theK;
    cv::Vec4d theD;
    std::vector<cv::Vec3d> rvec;
    std::vector<cv::Vec3d> tvec;

    cv::fisheye::calibrate(objectPoints, imagePoints, imageSize, theK, theD,
                           rvec, tvec, flag, cv::TermCriteria(3, 20, 1e-6));

    cv::internal::IntrinsicParams param, errors;
    cv::Vec2d err_std;
    double thresh_cond = 1e6;
    int check_cond = 1;
    param.Init(cv::Vec2d(theK(0,0), theK(1,1)), cv::Vec2d(theK(0,2), theK(1, 2)), theD);
    param.isEstimate = std::vector<uchar>(9, 1);
    param.isEstimate[4] = 0;

    errors.isEstimate = param.isEstimate;

    double rms;

    cv::internal::EstimateUncertainties(objectPoints, imagePoints, param,  rvec, tvec,
                                        errors, err_std, thresh_cond, check_cond, rms);

    EXPECT_MAT_NEAR(errors.f, cv::Vec2d(1.34250246865020720, 1.36037536429654530), 1e-10);
    EXPECT_MAT_NEAR(errors.c, cv::Vec2d(0.92070526160049848, 0.84383585812851514), 1e-10);
    EXPECT_MAT_NEAR(errors.k, cv::Vec4d(0.0053379581373996041, 0.017389792901700545, 0.022036256089491224, 0.0094714594258908952), 1e-10);
    EXPECT_MAT_NEAR(err_std, cv::Vec2d(0.187475975266883, 0.185678953263995), 1e-10);
    CV_Assert(fabs(rms - 0.263782587133546) < 1e-10);
    CV_Assert(errors.alpha == 0);
}

TEST_F(fisheyeTest, stereoRectify)
{
    // For consistency purposes
    CV_StaticAssert(
        static_cast<int>(cv::CALIB_ZERO_DISPARITY) == static_cast<int>(cv::fisheye::CALIB_ZERO_DISPARITY),
        "For the purpose of continuity the following should be true: cv::CALIB_ZERO_DISPARITY == cv::fisheye::CALIB_ZERO_DISPARITY"
    );

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");

    cv::Size calibration_size = this->imageSize, requested_size = calibration_size;
    cv::Matx33d K1 = this->K, K2 = K1;
    cv::Mat D1 = cv::Mat(this->D), D2 = D1;

    cv::Vec3d theT = this->T;
    cv::Matx33d theR = this->R;

    double balance = 0.0, fov_scale = 1.1;
    cv::Mat R1, R2, P1, P2, Q;
    cv::fisheye::stereoRectify(K1, D1, K2, D2, calibration_size, theR, theT, R1, R2, P1, P2, Q,
                      cv::fisheye::CALIB_ZERO_DISPARITY, requested_size, balance, fov_scale);

    // Collected with these CMake flags: -DWITH_IPP=OFF -DCV_ENABLE_INTRINSICS=OFF -DCV_DISABLE_OPTIMIZATION=ON -DCMAKE_BUILD_TYPE=Debug
    cv::Matx33d R1_ref(
        0.9992853269091279, 0.03779164101000276, -0.0007920188690205426,
        -0.03778569762983931, 0.9992646472015868, 0.006511981857667881,
        0.001037534936357442, -0.006477400933964018, 0.9999784831677112
    );
    cv::Matx33d R2_ref(
        0.9994868963898833, -0.03197579751378937, -0.001868774538573449,
        0.03196298186616116, 0.9994677442608699, -0.0065265589947392,
        0.002076471801477729, 0.006463478587068991, 0.9999769555891836
    );
    cv::Matx34d P1_ref(
        420.9684016542647, 0, 586.3059567784627, 0,
        0, 420.9684016542647, 374.8571836462291, 0,
        0, 0, 1, 0
    );
    cv::Matx34d P2_ref(
        420.9684016542647, 0, 586.3059567784627, -41.78881938824554,
        0, 420.9684016542647, 374.8571836462291, 0,
        0, 0, 1, 0
    );
    cv::Matx44d Q_ref(
        1, 0, 0, -586.3059567784627,
        0, 1, 0, -374.8571836462291,
        0, 0, 0, 420.9684016542647,
        0, 0, 10.07370889670733, -0
    );

    const double eps = 1e-10;
    EXPECT_MAT_NEAR(R1_ref, R1, eps);
    EXPECT_MAT_NEAR(R2_ref, R2, eps);
    EXPECT_MAT_NEAR(P1_ref, P1, eps);
    EXPECT_MAT_NEAR(P2_ref, P2, eps);
    EXPECT_MAT_NEAR(Q_ref, Q, eps);

    if (::testing::Test::HasFailure())
    {
        std::cout << "Actual values are:" << std::endl
            << "R1 =" << std::endl << R1 << std::endl
            << "R2 =" << std::endl << R2 << std::endl
            << "P1 =" << std::endl << P1 << std::endl
            << "P2 =" << std::endl << P2 << std::endl
            << "Q =" << std::endl << Q << std::endl;
    }

    if (cvtest::debugLevel == 0)
        return;
    // DEBUG code is below

    cv::Mat lmapx, lmapy, rmapx, rmapy;
    //rewrite for fisheye
    cv::fisheye::initUndistortRectifyMap(K1, D1, R1, P1, requested_size, CV_32F, lmapx, lmapy);
    cv::fisheye::initUndistortRectifyMap(K2, D2, R2, P2, requested_size, CV_32F, rmapx, rmapy);

    cv::Mat l, r, lundist, rundist;
    for (int i = 0; i < 34; ++i)
    {
        SCOPED_TRACE(cv::format("image %d", i));
        l = imread(combine(folder, cv::format("left/stereo_pair_%03d.jpg", i)), cv::IMREAD_COLOR);
        r = imread(combine(folder, cv::format("right/stereo_pair_%03d.jpg", i)), cv::IMREAD_COLOR);
        ASSERT_FALSE(l.empty());
        ASSERT_FALSE(r.empty());

        int ndisp = 128;
        cv::rectangle(l, cv::Rect(255,       0, 829,       l.rows-1), cv::Scalar(0, 0, 255));
        cv::rectangle(r, cv::Rect(255,       0, 829,       l.rows-1), cv::Scalar(0, 0, 255));
        cv::rectangle(r, cv::Rect(255-ndisp, 0, 829+ndisp ,l.rows-1), cv::Scalar(0, 0, 255));
        cv::remap(l, lundist, lmapx, lmapy, cv::INTER_LINEAR);
        cv::remap(r, rundist, rmapx, rmapy, cv::INTER_LINEAR);

        for (int ii = 0; ii < lundist.rows; ii += 20)
        {
            cv::line(lundist, cv::Point(0, ii), cv::Point(lundist.cols, ii), cv::Scalar(0, 255, 0));
            cv::line(rundist, cv::Point(0, ii), cv::Point(lundist.cols, ii), cv::Scalar(0, 255, 0));
        }

        cv::Mat rectification;
        merge4(l, r, lundist, rundist, rectification);

        // Add the "--test_debug" to arguments for file output
        if (cvtest::debugLevel > 0)
            cv::imwrite(cv::format("fisheye_rectification_AB_%03d.png", i), rectification);
    }
}

TEST_F(fisheyeTest, stereoCalibrate)
{
    const int n_images = 34;

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");

    std::vector<std::vector<cv::Point2d> > leftPoints(n_images);
    std::vector<std::vector<cv::Point2d> > rightPoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    cv::FileStorage fs_left(combine(folder, "left.xml"), cv::FileStorage::READ);
    CV_Assert(fs_left.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_left[cv::format("image_%d", i )] >> leftPoints[i];
    fs_left.release();

    cv::FileStorage fs_right(combine(folder, "right.xml"), cv::FileStorage::READ);
    CV_Assert(fs_right.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_right[cv::format("image_%d", i )] >> rightPoints[i];
    fs_right.release();

    cv::FileStorage fs_object(combine(folder, "object.xml"), cv::FileStorage::READ);
    CV_Assert(fs_object.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_object[cv::format("image_%d", i )] >> objectPoints[i];
    fs_object.release();

    cv::Matx33d K1, K2, theR;
    cv::Vec3d theT;
    cv::Vec4d D1, D2;

    int flag = 0;
    flag |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flag |= cv::fisheye::CALIB_CHECK_COND;
    flag |= cv::fisheye::CALIB_FIX_SKEW;

    cv::fisheye::stereoCalibrate(objectPoints, leftPoints, rightPoints,
                    K1, D1, K2, D2, imageSize, theR, theT, flag,
                    cv::TermCriteria(3, 12, 0));

    cv::Matx33d R_correct(   0.9975587205950972,   0.06953016383322372, 0.006492709911733523,
                           -0.06956823121068059,    0.9975601387249519, 0.005833595226966235,
                          -0.006071257768382089, -0.006271040135405457, 0.9999619062167968);
    cv::Vec3d T_correct(-0.099402724724121, 0.00270812139265413, 0.00129330292472699);
    cv::Matx33d K1_correct (561.195925927249,                0, 621.282400272412,
                                   0, 562.849402029712, 380.555455380889,
                                   0,                0,                1);

    cv::Matx33d K2_correct (560.395452535348,                0, 678.971652040359,
                                   0,  561.90171021422, 380.401340535339,
                                   0,                0,                1);

    cv::Vec4d D1_correct (-7.44253716539556e-05, -0.00702662033932424, 0.00737569823650885, -0.00342230256441771);
    cv::Vec4d D2_correct (-0.0130785435677431, 0.0284434505383497, -0.0360333869900506, 0.0144724062347222);

    EXPECT_MAT_NEAR(theR, R_correct, 1e-10);
    EXPECT_MAT_NEAR(theT, T_correct, 1e-10);

    EXPECT_MAT_NEAR(K1, K1_correct, 1e-10);
    EXPECT_MAT_NEAR(K2, K2_correct, 1e-10);

    EXPECT_MAT_NEAR(D1, D1_correct, 1e-10);
    EXPECT_MAT_NEAR(D2, D2_correct, 1e-10);

}

TEST_F(fisheyeTest, stereoCalibrateFixIntrinsic)
{
    const int n_images = 34;

    const std::string folder =combine(datasets_repository_path, "calib-3_stereo_from_JY");

    std::vector<std::vector<cv::Point2d> > leftPoints(n_images);
    std::vector<std::vector<cv::Point2d> > rightPoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    cv::FileStorage fs_left(combine(folder, "left.xml"), cv::FileStorage::READ);
    CV_Assert(fs_left.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_left[cv::format("image_%d", i )] >> leftPoints[i];
    fs_left.release();

    cv::FileStorage fs_right(combine(folder, "right.xml"), cv::FileStorage::READ);
    CV_Assert(fs_right.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_right[cv::format("image_%d", i )] >> rightPoints[i];
    fs_right.release();

    cv::FileStorage fs_object(combine(folder, "object.xml"), cv::FileStorage::READ);
    CV_Assert(fs_object.isOpened());
    for(int i = 0; i < n_images; ++i)
        fs_object[cv::format("image_%d", i )] >> objectPoints[i];
    fs_object.release();

    cv::Matx33d theR;
    cv::Vec3d theT;

    int flag = 0;
    flag |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flag |= cv::fisheye::CALIB_CHECK_COND;
    flag |= cv::fisheye::CALIB_FIX_SKEW;
    flag |= cv::fisheye::CALIB_FIX_INTRINSIC;

    cv::Matx33d K1 (561.195925927249,                0, 621.282400272412,
                                   0, 562.849402029712, 380.555455380889,
                                   0,                0,                1);

    cv::Matx33d K2 (560.395452535348,                0, 678.971652040359,
                                   0,  561.90171021422, 380.401340535339,
                                   0,                0,                1);

    cv::Vec4d D1 (-7.44253716539556e-05, -0.00702662033932424, 0.00737569823650885, -0.00342230256441771);
    cv::Vec4d D2 (-0.0130785435677431, 0.0284434505383497, -0.0360333869900506, 0.0144724062347222);

    cv::fisheye::stereoCalibrate(objectPoints, leftPoints, rightPoints,
                    K1, D1, K2, D2, imageSize, theR, theT, flag,
                    cv::TermCriteria(3, 12, 0));

    cv::Matx33d R_correct(   0.9975587205950972,   0.06953016383322372, 0.006492709911733523,
                           -0.06956823121068059,    0.9975601387249519, 0.005833595226966235,
                          -0.006071257768382089, -0.006271040135405457, 0.9999619062167968);
    cv::Vec3d T_correct(-0.099402724724121, 0.00270812139265413, 0.00129330292472699);


    EXPECT_MAT_NEAR(theR, R_correct, 1e-10);
    EXPECT_MAT_NEAR(theT, T_correct, 1e-10);
}

TEST_F(fisheyeTest, CalibrationWithDifferentPointsNumber)
{
    const int n_images = 2;

    std::vector<std::vector<cv::Point2d> > imagePoints(n_images);
    std::vector<std::vector<cv::Point3d> > objectPoints(n_images);

    std::vector<cv::Point2d> imgPoints1(10);
    std::vector<cv::Point2d> imgPoints2(15);

    std::vector<cv::Point3d> objectPoints1(imgPoints1.size());
    std::vector<cv::Point3d> objectPoints2(imgPoints2.size());

    for (size_t i = 0; i < imgPoints1.size(); i++)
    {
        imgPoints1[i] = cv::Point2d((double)i, (double)i);
        objectPoints1[i] = cv::Point3d((double)i, (double)i, 10.0);
    }

    for (size_t i = 0; i < imgPoints2.size(); i++)
    {
        imgPoints2[i] = cv::Point2d(i + 0.5, i + 0.5);
        objectPoints2[i] = cv::Point3d(i + 0.5, i + 0.5, 10.0);
    }

    imagePoints[0] = imgPoints1;
    imagePoints[1] = imgPoints2;
    objectPoints[0] = objectPoints1;
    objectPoints[1] = objectPoints2;

    cv::Matx33d theK = cv::Matx33d::eye();
    cv::Vec4d theD;

    int flag = 0;
    flag |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flag |= cv::fisheye::CALIB_USE_INTRINSIC_GUESS;
    flag |= cv::fisheye::CALIB_FIX_SKEW;

    cv::fisheye::calibrate(objectPoints, imagePoints, cv::Size(100, 100), theK, theD,
        cv::noArray(), cv::noArray(), flag, cv::TermCriteria(3, 20, 1e-6));
}

TEST_F(fisheyeTest, estimateNewCameraMatrixForUndistortRectify)
{
    cv::Size size(1920, 1080);

    cv::Mat K_fullhd(3, 3, cv::DataType<double>::type);
    K_fullhd.at<double>(0, 0) = 600.44477382;
    K_fullhd.at<double>(0, 1) = 0.0;
    K_fullhd.at<double>(0, 2) = 992.06425788;

    K_fullhd.at<double>(1, 0) = 0.0;
    K_fullhd.at<double>(1, 1) = 578.99298055;
    K_fullhd.at<double>(1, 2) = 549.26826242;

    K_fullhd.at<double>(2, 0) = 0.0;
    K_fullhd.at<double>(2, 1) = 0.0;
    K_fullhd.at<double>(2, 2) = 1.0;

    cv::Mat K_new_truth(3, 3, cv::DataType<double>::type);

    K_new_truth.at<double>(0, 0) = 387.5118215642316;
    K_new_truth.at<double>(0, 1) = 0.0;
    K_new_truth.at<double>(0, 2) = 1033.936556777084;

    K_new_truth.at<double>(1, 0) = 0.0;
    K_new_truth.at<double>(1, 1) = 373.6673784974842;
    K_new_truth.at<double>(1, 2) = 538.794152656429;

    K_new_truth.at<double>(2, 0) = 0.0;
    K_new_truth.at<double>(2, 1) = 0.0;
    K_new_truth.at<double>(2, 2) = 1.0;

    cv::Mat D_fullhd(4, 1, cv::DataType<double>::type);
    D_fullhd.at<double>(0, 0) = -0.05090103223466704;
    D_fullhd.at<double>(1, 0) = 0.030944413642173308;
    D_fullhd.at<double>(2, 0) = -0.021509225493198905;
    D_fullhd.at<double>(3, 0) = 0.0043378096628297145;
    cv::Mat E = cv::Mat::eye(3, 3, cv::DataType<double>::type);

    cv::Mat K_new(3, 3, cv::DataType<double>::type);

    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(K_fullhd, D_fullhd, size, E, K_new, 0.0, size);

    EXPECT_MAT_NEAR(K_new, K_new_truth, 1e-6);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  fisheyeTest::

const cv::Size fisheyeTest::imageSize(1280, 800);

const cv::Matx33d fisheyeTest::K(558.478087865323,               0, 620.458515360843,
                              0, 560.506767351568, 381.939424848348,
                              0,               0,                1);

const cv::Vec4d fisheyeTest::D(-0.0014613319981768, -0.00329861110580401, 0.00605760088590183, -0.00374209380722371);

const cv::Matx33d fisheyeTest::R ( 9.9756700084424932e-01, 6.9698277640183867e-02, 1.4929569991321144e-03,
                            -6.9711825162322980e-02, 9.9748249845531767e-01, 1.2997180766418455e-02,
                            -5.8331736398316541e-04,-1.3069635393884985e-02, 9.9991441852366736e-01);

const cv::Vec3d fisheyeTest::T(-9.9217369356044638e-02, 3.1741831972356663e-03, 1.8551007952921010e-04);

std::string fisheyeTest::combine(const std::string& _item1, const std::string& _item2)
{
    std::string item1 = _item1, item2 = _item2;
    std::replace(item1.begin(), item1.end(), '\\', '/');
    std::replace(item2.begin(), item2.end(), '\\', '/');

    if (item1.empty())
        return item2;

    if (item2.empty())
        return item1;

    char last = item1[item1.size()-1];
    return item1 + (last != '/' ? "/" : "") + item2;
}

void fisheyeTest::merge4(const cv::Mat& tl, const cv::Mat& tr, const cv::Mat& bl, const cv::Mat& br, cv::Mat& merged)
{
    int type = tl.type();
    cv::Size sz = tl.size();
    ASSERT_EQ(type, tr.type()); ASSERT_EQ(type, bl.type()); ASSERT_EQ(type, br.type());
    ASSERT_EQ(sz.width, tr.cols); ASSERT_EQ(sz.width, bl.cols); ASSERT_EQ(sz.width, br.cols);
    ASSERT_EQ(sz.height, tr.rows); ASSERT_EQ(sz.height, bl.rows); ASSERT_EQ(sz.height, br.rows);

    merged.create(cv::Size(sz.width * 2, sz.height * 2), type);
    tl.copyTo(merged(cv::Rect(0, 0, sz.width, sz.height)));
    tr.copyTo(merged(cv::Rect(sz.width, 0, sz.width, sz.height)));
    bl.copyTo(merged(cv::Rect(0, sz.height, sz.width, sz.height)));
    br.copyTo(merged(cv::Rect(sz.width, sz.height, sz.width, sz.height)));
}

}} // namespace
