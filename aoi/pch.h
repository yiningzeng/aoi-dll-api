// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <opencv2/opencv.hpp>
// 图像拼接用的对齐边的定义
enum side { none = 0, left = 1, up = 2, right = 4, down = 8 };
#ifdef __cplusplus //(内置宏,如果是c++,在编译器预处理的时候加上extern,如果是c语言调用的时候是不处理的)
extern "C"
{
#endif

	__declspec(dllexport) void hello(cv::Mat& img);
	/////////////////////////////////////////////////////////////////
	//                      基本确定的接口                         //
	/////////////////////////////////////////////////////////////////

	// 抽色
	// gray：灰度图灰度图
	// rgb：灰度图rgb 彩色图
	// mask：灰度图输出的掩码图
	// params：灰度图参数数组，依次为灰度的阈值下限、上限，r 通道的下限、上限，g 下限、上限，b 下限、上限
	__declspec(dllexport) void range_mask(const cv::Mat& gray, const cv::Mat& rgb, cv::Mat& mask, const int* params);

	// 二维码识别算子
	// gray：灰度图灰度图
	// symbol_type：灰度图需要识别的条形码/二维码类型，参考
	// http://zbar.sourceforge.net/api/zbar_8h.html#f7818ad6458f9f40362eecda97acdcb0
	//std::string decode(const cv::Mat& gray, const zbar::zbar_symbol_type_t symbol_type=zbar::ZBAR_NONE);

	// 灰度图生成直方图
	// gray：灰度图灰度图
	// n_bins：灰度图柱子的个数
	// hist：灰度图输出的直方图，n_bins x 1 的矩阵，类型为 CV_32F 的，hist.data 是数据指针，float32 型的
	__declspec(dllexport) void histogram(const cv::Mat& gray, const int n_bins, cv::Mat& hist);


	/////////////////////////////////////////////////////////////////
	//                      改写中的接口                           //
	/////////////////////////////////////////////////////////////////

	// 图像拼接（简单的基于已知相对位置的拼接）
	// img: 拼接后的目标大图（输出），内存必须在调用之前预留好，不能为空
	// tl: 新加进去的小图的左上角顶点在大图中的位置
	// patch: 新加进去的单张小图
	__declspec(dllexport) void add_patch(cv::Mat& img, const cv::Point& tl, const cv::Mat& patch);

	__declspec(dllexport) void copy_to(cv::Mat& img, cv::Mat& patch, const cv::Rect& roi_ref);

	__declspec(dllexport) void stitch_v2(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2);

	// 图像拼接
	// img: 拼接后的大图（输出）
	// roi_ref: 用做对齐用的（在大图中已经存在的）参考区域 参考区域 上一张图片在大图中的区域
	// patch: 新加进去的单张小图
	// roi_patch: 新加的小图在大图中的位置（输出）  新图所在的区域，用于下一张拼接的图的参考区域
	// side1: 新加的小图相对于参照区域坐在的位置：灰度图右：灰度图0;下：灰度图1;左：灰度图2;上：灰度图3
	// overlap_lb1: 图像重叠区域宽度的下限，以像素为单位。（至少重叠 overlap_up 个像素）
	// overlap_ub1: 图像重叠区域宽度的上限，以像素为单位。（即至多重叠 overlap_up 个像素）
	// drift_ub1: 在拼接方向的垂直方向上的错位的上限，以像素为单位。 设置一个默认值 待陈哥告知
	// side2: 使用两边做对齐时的第二个对齐边，使用单边对齐（默认）时，设置为0
	// overlap_lb2, overlap_ub2, drift_ub2 对应于第二个对齐边相应的值, side2=0时无效
	// overlap_lb, overlap_ub, drift_ub 需要根据机械精度做相应的设置
	__declspec(dllexport) void stitch(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2);
	// 通过 marker 位置做旋转对齐矫正，需要两个点 p、q 在旋转前/后的对应坐标
	// _p, _q: 旋转前 p, q 的二维坐标
	// p_, q_: 旋转后的坐标
	// ret: 输出的参数，五个 double
	__declspec(dllexport) void get_rotation_parameters(const double* _p, const double* _q, const double* p_, const double* q_, double* ret);

	// 从旋转参数计算旋转变换矩阵
	// rot: 旋转参数，即 get_rotation_parameters 输出的数组
	// rmat: 旋转矩阵（输出）
	__declspec(dllexport) void get_rotation_matrix_2d(const double* rot, cv::Mat& rmat);

	/////////////////////////////////////////////////////////////////
	//                   以下部分暂时不需要                        //
	/////////////////////////////////////////////////////////////////

	// 求灰度图均值
	__declspec(dllexport) bool mean(const cv::Mat& gray, const int* params);

	// 像素最大，最小，范围判定
	__declspec(dllexport) bool min_max_range(const cv::Mat& gray, const int* params, unsigned mode = 0);

	// scale 算子
	__declspec(dllexport) bool scale(const cv::Mat& gray, const cv::Mat& rgb, const int* params);

	__declspec(dllexport) double image_match(const cv::Mat& img, const cv::Mat& templ, cv::Point* pos = NULL, bool binarize = false);
#ifdef __cplusplus
}
#endif
#endif //PCH_H
