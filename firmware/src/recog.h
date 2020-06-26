#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/ml.hpp>

#include <unistd.h>
#include "feature.h"
#include <iostream>

using namespace cv;
using namespace std;
using namespace cv::ml;

void draw_rotated_rectangle(Mat img, RotatedRect rr, Scalar color, int thickness)
{
    Point2f vertices[4];
    rr.points(vertices);
    for (int i = 0; i < 4; i++)
    {
        line(img, vertices[i], vertices[(i + 1) % 4], color, thickness);
    }
}

char recognise_character(Mat img_character)
{
    // Load SVM training file OpenCV 3.1
    Ptr<SVM> svmNew = SVM::create();
    svmNew = SVM::load("svm.txt");
    char c = '*';

    vector<float> feature = calculate_feature(img_character);

    Mat m = Mat(1, number_of_feature, CV_32FC1);
    for (size_t i = 0; i < feature.size(); ++i)
    {
        float temp = feature[i];
        m.at<float>(0, i) = temp;
    }

    int ri = (int)(svmNew->predict(m)); // Open CV 3.1

    if (ri >= 0 && ri < 10)
        c = (char)(ri + 48);
    if (ri >= 10 && ri < 18)
        c = (char)(ri + 55);
    if (ri >= 18)
        c = (char)(ri + 56);

    return c;
}

void extract_patch_from_img(Mat &src, Mat &dst, RotatedRect rr)
{
    Rect r = rr.boundingRect();

    if (r.x >= 0 && r.y >= 0 &&
        (r.x + r.width) < src.cols &&
        (r.y + r.height) < src.rows)
    {
        Mat precrop = src(r);

        int cropMidX, cropMidY;
        cropMidX = r.width / 2;
        cropMidY = r.height / 2;

        Size2f rr_size = rr.size;
        int width = (int)rr_size.width;
        int height = (int)rr_size.height;

        Mat map_mat = getRotationMatrix2D(Point2f(cropMidX, cropMidY), rr.angle, 1.0f);
        map_mat.at<double>(0, 2) += static_cast<double>(width / 2 - cropMidX);
        map_mat.at<double>(1, 2) += static_cast<double>(height / 2 - cropMidY);

        warpAffine(precrop, dst, map_mat, Size2i(width, height));

        if (rr.size.width < rr.size.height)
        {
            float a = 90;
            Point2f center((dst.cols - 1) / 2.0, (dst.rows - 1) / 2.0);
            Mat rot = getRotationMatrix2D(center, a, 1.0f);

            Rect2f bbox = RotatedRect(Point2f(), dst.size(), a).boundingRect2f();
            rot.at<double>(0, 2) += bbox.width / 2.0 - dst.cols / 2.0;
            rot.at<double>(1, 2) += bbox.height / 2.0 - dst.rows / 2.0;

            warpAffine(dst, dst, rot, bbox.size());
        }
    }
}

vector<Mat> find_characters(Mat src_image, int block_size)
{
    vector<Mat> characters;
    Mat gray, binary;

    vector<vector<cv::Point>> contours;
    vector<Vec4i> hierarchy;

    cvtColor(src_image, gray, COLOR_BGR2GRAY);
    adaptiveThreshold(gray, binary, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, block_size, 3);

    Mat binary_clone = binary.clone();

    Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));

    erode(binary, binary, element);
    erode(binary, binary, element);
    dilate(binary, binary, element);

    imshow("binary", binary);

    findContours(binary, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

    if (contours.size() == 0)
        return characters;

    for (size_t i = 0; i < contours.size(); ++i)
    {
        // create bounding rect
        Rect r = boundingRect(contours.at(i));

        // discard if strangely sized
        if (r.width > src_image.cols / 2 || r.height > src_image.rows / 2 || r.width < 120 || r.height < 20 || (double)r.width / r.height < 3.5 || (double)r.width / r.height > 5.5)
            continue;

        RotatedRect rr = minAreaRect(contours.at(i));

        Mat sub_binary;
        extract_patch_from_img(binary_clone, sub_binary, rr);

        if (sub_binary.empty())
            continue;

        Mat plate = sub_binary.clone();

        // create contours inside plate
        vector<vector<cv::Point>> sub_contours;
        vector<Vec4i> sub_hierarchy;

        findContours(sub_binary, sub_contours, sub_hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

        if (sub_contours.size() < 8)
            continue;

        int count = 0;
        vector<Mat> c;

        // created rect around potential plate and checked for sub contours
        Mat sub_image = src_image(r);
        vector<Rect> r_characters;
        for (size_t j = 0; j < sub_contours.size(); ++j)
        {
            // bounding rect for a character
            Rect sub_r = boundingRect(sub_contours.at(j));

            // TODO: maybe add check the sub_r.width > r.width / 16
            if (sub_r.height > r.height / 2 && sub_r.width < r.width / 8 && sub_r.width > 10 && sub_r.x > 5)
            {
                Mat cj = plate(sub_r);

                double ratio = (double)count_pixel(cj) / (cj.cols * cj.rows);
                if (ratio > 0.2)
                {
                    r_characters.push_back(sub_r);
                    // rectangle(sub_image, sub_r, Scalar(0, 0, 255), 2, 8, 0);
                }
            }
        }

        // Order chars by x pos
        if (r_characters.size() >= 3)
        {
            for (int i = 0; i < r_characters.size() - 1; ++i)
            {
                for (int j = i + 1; j < r_characters.size(); ++j)
                {
                    Rect temp;
                    if (r_characters.at(j).x < r_characters.at(i).x)
                    {
                        temp = r_characters.at(j);
                        r_characters.at(j) = r_characters.at(i);
                        r_characters.at(i) = temp;
                    }
                }
            }

            for (int i = 0; i < r_characters.size(); ++i)
            {
                Mat ch = plate(r_characters.at(i));
                characters.push_back(ch);
            }

            draw_rotated_rectangle(src_image, rr, Scalar(0, 255, 0), 2);
            imshow("plate", plate);
            break;
        }

        draw_rotated_rectangle(src_image, rr, Scalar(0, 255, 0), 2);
    }

    imshow("frame", src_image);

    return characters;
}

vector<string> predict(Mat src_image)
{
    vector<string> results;

    for (size_t i = 0; i < 3; ++i)
    {
        string result = "";
        vector<Mat> characters = find_characters(src_image, (i * 100) + 75);

        for (size_t j = 0; j < characters.size(); ++j)
        {
            char ch = recognise_character(characters.at(j));
            result.push_back(ch);
        }

        results.push_back(result);
    }

    return results;
}
