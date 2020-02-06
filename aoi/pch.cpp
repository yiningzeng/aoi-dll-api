﻿// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"

#include <iostream>

#define INRANGE(VAL, LB, UB) (VAL >= LB &&VAL <= UB)

//params format: lower/upper
bool mean(const cv::Mat& gray, const int* params)
{
    int mean = cv::mean(gray)[0];
    return INRANGE(mean, params[0], params[1]);
}

//params format: sigma_x, sigma_y, lower, upper
//mode: 1 -> min; 2 -> max; 3 -> range;
bool min_max_range(const cv::Mat& gray, const int* params, unsigned mode)
{
    assert(mode < 3);

    cv::Mat img;
    cv::blur(gray, img, cv::Size(params[0], params[1]));
    double val[3];
    cv::minMaxLoc(img, &val[0], &val[1]);
    val[2] = val[1] - val[0];

    return INRANGE(val[mode], params[2], params[3]);
}

//params format: gray/red/green/blue/decision x lower/upper
void range_mask(const cv::Mat& gray, const cv::Mat& rgb, cv::Mat& mask, const int* params)
{
    assert(gray.channels() == 1);
    assert(rgb.channels() == 3);

    cv::Mat mask_gray;
    cv::inRange(gray, params[0], params[1], mask_gray);

    cv::Mat mask_rgb;
    cv::Scalar lower(params[6], params[4], params[2]);
    cv::Scalar upper(params[7], params[5], params[3]);
    cv::inRange(rgb, lower, upper, mask_rgb);

    cv::bitwise_and(mask_rgb, mask_gray, mask);
}

bool scale(const cv::Mat& gray, const cv::Mat& rgb, const int* params)
{
    cv::Mat mask;
    range_mask(gray, rgb, mask, params);
    //cv::imshow("CV: mask", mask);

    int sum = cv::sum(mask)[0] / 255;
    int percentage = sum * 100 / mask.total();

    return INRANGE(percentage, params[8], params[9]);
}

// returns top left corner
double image_match(const cv::Mat& img, const cv::Mat& templ, cv::Point* pos, bool binarize)
{
    cv::Mat result;
    if (binarize)
    {
        cv::Mat img_bin, templ_bin;
        if (img.channels() != 1)
        {
            cv::cvtColor(img, img_bin, cv::COLOR_BGR2GRAY);
        }
        else
        {
            img_bin = img.clone();
        }
        if (templ.channels() != 1)
        {
            cv::cvtColor(templ, templ_bin, cv::COLOR_BGR2GRAY);
        }
        else
        {
            templ_bin = templ.clone();
        }

        double thresh = cv::threshold(img_bin, img_bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::threshold(templ_bin, templ_bin, thresh, 255, cv::THRESH_BINARY);
        cv::matchTemplate(img_bin, templ_bin, result, cv::TM_CCOEFF_NORMED);
    }
    else
    {
        if (img.channels() != templ.channels())
        {
            cv::Mat img_mono, templ_mono;
            if (img.channels() != 1)
            {
                cv::cvtColor(img, img_mono, cv::COLOR_BGR2GRAY);
            }
            if (templ.channels() != 1)
            {
                cv::cvtColor(templ, templ_mono, cv::COLOR_BGR2GRAY);
            }
            cv::matchTemplate(img_mono, templ_mono, result, cv::TM_CCOEFF_NORMED);
        }
        else
        {
            cv::matchTemplate(img, templ, result, cv::TM_CCOEFF_NORMED);
        }
    }

    double maxVal;
    cv::minMaxLoc(result, NULL, &maxVal, NULL, pos);

    return maxVal;
}

void histogram(const cv::Mat& gray, const int n_bins, cv::Mat& hist)
{
    float range[] = { 0, 256 }; //the upper boundary is exclusive
    const float* histRange = { range };
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &n_bins, &histRange);
}

bool connex(const cv::Mat& img, const int* params)
{
    double minVal, maxVal;
    cv::minMaxLoc(img, &minVal, &maxVal);

    return minVal >= params[0] && maxVal <= params[1];
}

// decode barcode, QRcode
//std::string decode(const cv::Mat& gray, const zbar::zbar_symbol_type_t symbol_type)
//{
//zbar::Image image(gray.cols, gray.rows, "Y800", (uchar *)(gray.data), gray.total());

//zbar::ImageScanner scanner;
//scanner.set_config(symbol_type, zbar::ZBAR_CFG_ENABLE, 1);
//scanner.scan(image);

//if(image.symbol_begin() == image.symbol_end()) return "";

//zbar::SymbolIterator symbol = image.symbol_begin();
////std::string type = symbol->get_type_name();
//return symbol->get_data();
//}

void get_rotation_parameters(const double* _p, const double* _q, const double* p_, const double* q_, double* ret)
{
    double _v[2]{ _p[0] - _q[0], _p[1] - _q[1] };
    double v_[2]{ p_[0] - q_[0], p_[1] - q_[1] };
    double n1 = std::sqrt((_v[0] * _v[0] + _v[1] * _v[1]));
    double n2 = std::sqrt((v_[0] * v_[0] + v_[1] * v_[1]));
    double norm = n1 * n2;
    ret[2] = (_v[0] * v_[0] + _v[1] * v_[1]) / norm;
    ret[3] = (_v[1] * v_[0] - _v[0] * v_[1]) / norm;
    ret[4] = n2 / n1;

    cv::Mat R = (cv::Mat_<double>(2, 2, CV_64F) << ret[2], ret[3], -ret[3], ret[2]);
    cv::Mat _c = (cv::Mat_<double>(2, 1, CV_64F) << (_p[0] + _q[0]) / 2, (_p[1] + _q[1]) / 2);
    cv::Mat c_ = (cv::Mat_<double>(2, 1, CV_64F) << (p_[0] + q_[0]) / 2, (p_[1] + q_[1]) / 2);

    cv::Mat c = (R - cv::Mat::eye(2, 2, CV_64F)).inv() * (R * _c - c_);
    *(cv::Point2d*)ret = c.at<cv::Point2d>(0);
}

void get_rotation_matrix_2d(const double* rot, cv::Mat& rmat)
{
    double alpha = rot[4] * rot[2];
    double beta = rot[4] * rot[3];
    rmat = (cv::Mat_<double>(2, 3, CV_64F) <<
        alpha, beta, (1 - alpha) * rot[0] - beta * rot[1],
        -beta, alpha, beta * rot[0] + (1 - alpha) * rot[1]);
}

cv::Rect get_template_roi(cv::Size size, int side, int overlap_lb, int overlap_ub, int drift_ub)
{
    cv::Rect templ;
    switch (side)
    {
    case side::left:
        templ.x = 0;
        templ.y = drift_ub;
        templ.width = overlap_lb;
        templ.height = size.height - drift_ub * 2;
        break;
    case side::up:
        templ.x = drift_ub;
        templ.y = 0;
        templ.width = size.width - drift_ub * 2;
        templ.height = overlap_lb;
        break;
    case side::right:
        templ.x = size.width - overlap_lb;
        templ.y = drift_ub;
        templ.width = overlap_lb;
        templ.height = size.height - drift_ub * 2;
        break;
    case side::down:
        templ.x = drift_ub;
        templ.y = size.height - overlap_lb;
        templ.width = size.width - drift_ub * 2;
        templ.height = overlap_lb;
        break;
    }
    return templ;
}

cv::Rect get_search_roi(cv::Rect roi_ref, int side, int overlap_ub)
{
    cv::Rect roi_search;
    switch (side)
    {
    case side::left:
        roi_search.x = roi_ref.x + roi_ref.width - overlap_ub;
        roi_search.y = roi_ref.y;
        roi_search.width = overlap_ub;
        roi_search.height = roi_ref.height;
        break;
    case side::up:
        roi_search.x = roi_ref.x;
        roi_search.y = roi_ref.y + roi_ref.height - overlap_ub;
        roi_search.width = roi_ref.width;
        roi_search.height = overlap_ub;
        break;
    case side::right:
        roi_search.x = roi_ref.x;
        roi_search.y = roi_ref.y;
        roi_search.width = overlap_ub;
        roi_search.height = roi_ref.height;
        break;
    case side::down:
        roi_search.x = roi_ref.x;
        roi_search.y = roi_ref.y;
        roi_search.width = roi_ref.width;
        roi_search.height = overlap_ub;
        break;
    }
    return roi_search;
}

// match side1 and side2 simultaneously
void stitch_simul(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2)
{
    assert(side1 == side::left || side1 == side::up || side1 == side::right || side1 == side::down);
    assert(side2 == side::none || side2 == side::left || side2 == side::up || side2 == side::right || side2 == side::down);

    cv::Rect templ1(get_template_roi(patch.size(), side1, overlap_lb1, overlap_ub1, drift_ub1));
    cv::Rect roi_search1 = get_search_roi(roi_ref, side1, overlap_ub1);
    cv::Mat response1;
    cv::matchTemplate(img(roi_search1), patch(templ1), response1, cv::TM_CCOEFF);

    if (side2)
    {
        cv::Rect templ2(get_template_roi(patch.size(), side2, overlap_lb2, overlap_ub2, drift_ub2));
        cv::Rect roi_search2(roi_search1.tl() + templ2.tl() - templ1.tl(), roi_search1.size() - templ1.size() + templ2.size());

        cv::Mat response2;
        cv::matchTemplate(img(roi_search2), patch(templ2), response2, cv::TM_CCOEFF);
        response1 += response2;
    }

    cv::Point tl;
    cv::minMaxLoc(response1, NULL, NULL, NULL, &tl);

    roi_patch = cv::Rect(roi_search1.tl() + tl - templ1.tl(), patch.size());
}

// match side2 only for refining along one axis
void stitch_refine(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2)
{
    assert(side1 == side::left || side1 == side::up || side1 == side::right || side1 == side::down);
    assert(side2 == side::none || side2 == side::left || side2 == side::up || side2 == side::right || side2 == side::down);

    cv::Rect templ1(get_template_roi(patch.size(), side1, overlap_lb1, overlap_ub1, drift_ub1));
    cv::Rect roi_search1 = get_search_roi(roi_ref, side1, overlap_ub1);
    cv::Point tl1;
    image_match(img(roi_search1), patch(templ1), &tl1);

    if (side2)
    {
        cv::Rect templ2(get_template_roi(patch.size(), side2, overlap_lb2, overlap_ub2, drift_ub2));

        cv::Rect roi_search2;
        roi_search2 = cv::Rect(roi_search1.tl() + templ2.tl() - templ1.tl(), templ2.size());
        switch (side1)
        {
        case side::left:
        case side::right:
            roi_search2.y += tl1.y;
            roi_search2.width += roi_search1.width - templ1.width;
            break;
        case side::up:
        case side::down:
            roi_search2.x += tl1.x;
            roi_search2.height += roi_search1.height - templ1.height;
            break;
        }

        cv::Point tl2;
        image_match(img(roi_search2), patch(templ2), &tl2);

        switch (side1)
        {
        case side::left:
        case side::right:
            tl1.x = tl2.x;
            break;
        case side::up:
        case side::down:
            tl1.y = tl2.y;
            break;
        }
    }

    roi_patch = cv::Rect(roi_search1.tl() + tl1 - templ1.tl(), patch.size());
}

void stitch(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2)
{
    stitch_simul(img, roi_ref, patch, roi_patch, side1, overlap_lb1, overlap_ub1, drift_ub1, side2, overlap_lb2, overlap_ub2, drift_ub2);
}

void add_patch(cv::Mat& img, const cv::Point& tl, const cv::Mat& patch)
{
    patch.copyTo(img(cv::Rect(tl, patch.size())));
}

void copy_to(cv::Mat& img, cv::Mat& patch, const cv::Rect& roi_ref)
{
    patch.copyTo(img(roi_ref));
}

void stitch_v2(cv::Mat& img, const cv::Rect& roi_ref, const cv::Mat& patch, cv::Rect& roi_patch, int side1, int overlap_lb1, int overlap_ub1, int drift_ub1, int side2, int overlap_lb2, int overlap_ub2, int drift_ub2)
{
    stitch_simul(img, roi_ref, patch, roi_patch, side1, overlap_lb1, overlap_ub1, drift_ub1, side2, overlap_lb2, overlap_ub2, drift_ub2);
}


void take_diff(const cv::Mat& img, const cv::Mat& ref, cv::Mat& diff, int sensitivity = 3, int min_area = 9)
{
    cv::bitwise_xor(img, ref, diff);

    cv::Mat structure_element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(sensitivity, sensitivity));
    cv::morphologyEx(diff, diff, cv::MORPH_OPEN, structure_element);
    cv::morphologyEx(diff, diff, cv::MORPH_CLOSE, structure_element);

    cv::Mat labels, stats, centroids;
    cv::connectedComponentsWithStats(diff, labels, stats, centroids, 8, CV_32S);
    std::vector<cv::Rect> components;
    for (int i = 1; i < stats.rows; ++i)
    {
        int* stat = stats.ptr<int>(i);
        if (stat[cv::CC_STAT_AREA] < min_area) continue;

        components.push_back(cv::Rect(stat[cv::CC_STAT_LEFT], stat[cv::CC_STAT_TOP], stat[cv::CC_STAT_WIDTH], stat[cv::CC_STAT_HEIGHT]));
        cv::rectangle(diff, components.back(), cv::Scalar(255));
    }
}

void measure_distance()
{
    //cv::distanceTransform
    //masking
}


void hello(cv::Mat& img) {
    std::cout << "Hello, World!" << std::endl;
    cv::imshow("test", img);

    cv::waitKey(0);
}

int main() {
    std::cout << "Hello, World!" << std::endl;
}
// 当使用预编译的头时，需要使用此源文件，编译才能成功。
