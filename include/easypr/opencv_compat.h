#ifndef OPENCV_COMPAT_H
#define OPENCV_COMPAT_H

// 此文件处理OpenCV 3.x和4.x之间的兼容性

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/ml.hpp>

// OpenCV 4.x compatibility defines
#if CV_VERSION_MAJOR >= 4
  // 旧常量替换为新常量
  #define CV_WINDOW_AUTOSIZE cv::WINDOW_AUTOSIZE
  #define CV_THRESH_BINARY cv::THRESH_BINARY
  #define CV_THRESH_OTSU cv::THRESH_OTSU
  #define CV_CHAIN_APPROX_NONE cv::CHAIN_APPROX_NONE
  #define CV_CHAIN_APPROX_SIMPLE cv::CHAIN_APPROX_SIMPLE
  #define CV_BGR2GRAY cv::COLOR_BGR2GRAY
  #define CV_RGB2GRAY cv::COLOR_RGB2GRAY
  #define CV_GRAY2BGR cv::COLOR_GRAY2BGR
  #define CV_BGR2HSV cv::COLOR_BGR2HSV
  #define CV_HSV2BGR cv::COLOR_HSV2BGR
  #define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
  #define CV_RETR_EXTERNAL cv::RETR_EXTERNAL
  #define CV_RETR_LIST cv::RETR_LIST
  #define CV_RETR_TREE cv::RETR_TREE
  #define CV_INTER_LINEAR cv::INTER_LINEAR
  #define CV_FILLED cv::FILLED
  #define CV_AA cv::LINE_AA
  #define CV_TM_SQDIFF cv::TM_SQDIFF
  #define CV_RGB cv::Scalar
  #define CV_INTER_CUBIC cv::INTER_CUBIC
  #define CV_FONT_HERSHEY_SIMPLEX cv::FONT_HERSHEY_SIMPLEX
  #define CV_KMEANS_PP_CENTERS cv::KMEANS_PP_CENTERS
  #define CV_MOP_CLOSE cv::MORPH_CLOSE
  #define CV_MOP_OPEN cv::MORPH_OPEN
  #define CV_ADAPTIVE_THRESH_MEAN_C cv::ADAPTIVE_THRESH_MEAN_C

  // 类型转换
  namespace cv {
    typedef Ptr<ml::SVM> SVMPtr;
    typedef Ptr<ml::ANN_MLP> ANNMLPPtr;
  }

  // 函数重新定义
  #define cvRectangle cv::rectangle
  #define cvLine cv::line
  #define cvRound cv::saturate_cast<int>
  #define cvScalar cv::Scalar
  #define cvPoint cv::Point
  #define cvSize cv::Size
  #define cvPoint2f cv::Point2f
  #define cvCreateMat(rows, cols, type) cv::Mat(rows, cols, type)
  #define cvCloneImage(img) img.clone()
  #define cvResize(src, dst, interpolation) cv::resize(src, dst, dst.size(), 0, 0, interpolation)
  #define cvSmooth(src, dst, type, size1, size2) cv::GaussianBlur(src, dst, cv::Size(size1, size1), 0, 0)
#endif

#if CV_VERSION_MAJOR >= 4
  #define CV_LOAD_IMAGE_COLOR cv::IMREAD_COLOR
  #define CV_LOAD_IMAGE_GRAYSCALE cv::IMREAD_GRAYSCALE
#endif

// 定义外部变量替代
#ifndef CV_EXTERNAL
  #define CV_EXTERNAL 1
#endif

#ifndef CV_CHAIN_APPROX_NONE
  #define CV_CHAIN_APPROX_NONE 1
#endif

#ifndef CV_CHAIN_APPROX_SIMPLE
  #define CV_CHAIN_APPROX_SIMPLE 2
#endif

#ifndef CV_RGB
  #define CV_RGB cv::Scalar
#endif

#ifndef CV_THRESH_OTSU
  #define CV_THRESH_OTSU 8
#endif

#ifndef CV_THRESH_BINARY
  #define CV_THRESH_BINARY 0
#endif

#ifndef CV_RETR_EXTERNAL
  #define CV_RETR_EXTERNAL 0
#endif

#ifndef CV_RETR_LIST
  #define CV_RETR_LIST 1
#endif

#ifndef CV_RETR_TREE
  #define CV_RETR_TREE 3
#endif

// 颜色转换枚举 - 特定于EasyPR的定义
namespace easypr {
  enum COLOR_TYPE {
    COLOR_RGB2GRAY = 0,
    COLOR_BGR2GRAY = 1,
    COLOR_GRAY2BGR = 2,
    COLOR_BGRA2BGR = 3,
    COLOR_BGR2HSV = 4,
    COLOR_BGR2HSI = 5,
  };
}

#endif // OPENCV_COMPAT_H 