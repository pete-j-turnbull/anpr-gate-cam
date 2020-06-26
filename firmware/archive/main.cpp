#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <tesseract/baseapi.h>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;
using namespace cv;

string image_path = "/Users/pete/Development/iot/anpr-gate-cam/firmware/plate4.jpg";

// VideoCapture cap(0);

bool compareContourAreas(vector<Point> contour1, vector<Point> contour2)
{
    double i = fabs(contourArea(Mat(contour1)));
    double j = fabs(contourArea(Mat(contour2)));
    return (i > j);
}

void color_reduce(Mat &frame)
{
    int nl = frame.rows;
    int nc = frame.cols;

    for (int j = 0; j < nl; j++)
    {
        uchar *data = frame.ptr<uchar>(j);

        for (int i = 0; i < nc; i++)
        {
            int idx = i * 3;
            int total = data[idx] + data[idx + 1] + data[idx + 2];
            data[idx] = total > 382 ? 255 : 0;
            data[idx + 1] = total > 382 ? 255 : 0;
            data[idx + 2] = total > 382 ? 255 : 0;
        }
    }
}

void extract_hue(Mat &input_frame, Mat &output_frame)
{
    Mat hsv;
    vector<cv::Mat> channels;

    cvtColor(input_frame, hsv, COLOR_BGR2HSV);
    split(hsv, channels);

    output_frame = channels[0];
}
void shift_hue(Mat &frame, int shift)
{
    for (int j = 0; j < frame.rows; ++j)
        for (int i = 0; i < frame.cols; ++i)
        {
            frame.at<unsigned char>(j, i) = (frame.at<unsigned char>(j, i) + shift) % 180;
        }
}

void extract_value(Mat &input_frame, Mat &output_frame)
{
    Mat hsv;
    vector<cv::Mat> channels;

    cvtColor(input_frame, hsv, COLOR_BGR2HSV);
    split(hsv, channels);

    output_frame = channels[2];
}

void process_frame(Mat &input_frame, Mat &output_frame)
{
    Mat p_frame, resized, blurred, edges;

    // extract_hue(input_frame, hue);
    // shift_hue(hue, 50);

    // Canny(hue, edges, 50, 200, 3);
    // threshold(hue, hue_mask, 80, 255, THRESH_BINARY_INV);

    extract_value(input_frame, p_frame);
    medianBlur(input_frame, blurred, 15);

    // inRange(blurred, Scalar(210, 210, 210), Scalar(255, 255, 255), blurred);
    threshold(blurred, blurred, 127, 255, THRESH_BINARY);

    // bilateralFilter(p_frame, blurred, 11, 17, 17);
    // blurred = Scalar::all(255) - blurred;

    // adaptiveThreshold(blurred, edges, 255.0, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 9, 3);

    Canny(blurred, edges, 50, 200, 3);

    int dilation_size = 3;
    Mat element = getStructuringElement(MORPH_RECT,
                                        Size(2 * dilation_size + 1, 2 * dilation_size + 1),
                                        Point(dilation_size, dilation_size));
    dilate(edges, edges, element);

    // Mat h_frame(edges.size(), CV_8UC3, Scalar(0, 0, 0));
    // cvtColor(edges, h_frame, COLOR_GRAY2BGR);

    // int blockSize = 2;
    // int apertureSize = 3;
    // double k = 0.04;
    // cornerHarris(edges, h_frame, blockSize, apertureSize, k);

    // vector<Vec4i> lines;
    // HoughLinesP(edges, lines, 1, CV_PI / 90, 80, 30, 10);

    // cout << lines.size() << endl;
    // for (size_t i = 0; i < lines.size(); i++)
    // {
    //     line(h_frame, Point(lines[i][0], lines[i][1]),
    //          Point(lines[i][2], lines[i][3]), Scalar(0, 0, 255), 5, 8);
    // }

    vector<vector<Point>> contours;
    findContours(edges, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
    sort(contours.begin(), contours.end(), compareContourAreas);

    vector<vector<Point>> contours_poly(contours.size());
    vector<Point> screen_contour;
    Rect screen_bounds;

    Mat frame_contours(edges.size(), CV_8UC1, Scalar(0));
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        if (area < 50000)
            continue;

        int acc = 0.018 * arcLength(contours[i], true);
        approxPolyDP(Mat(contours[i]), contours_poly[i], acc, true);

        if (contours_poly[i].size() == 4)
        {
            Rect rect_bounds = boundingRect(contours_poly[i]);
            double area_ratio = contourArea(contours_poly[i]) / rect_bounds.area();
            cout << area_ratio << endl;

            if (area_ratio > 0.8)
            {
                screen_contour = contours_poly[i];

                //

                screen_bounds = boundingRect(contours_poly[i]);
                // drawContours(frame_contours, contours_poly, i, Scalar(255), 2);
                break;
            }
        }
    }

    Mat mask(input_frame.size(), CV_8UC1, Scalar(0));
    Rect padded_screen_bounds;

    if (!screen_contour.empty())
    {
        vector<vector<Point>> tmp;
        tmp.push_back(screen_contour);
        drawContours(mask, tmp, 0, 255, -1);

        int w_pad = screen_bounds.width * 0;
        int h_pad = screen_bounds.height * 0;
        padded_screen_bounds = Rect(screen_bounds.tl() + Point(w_pad, h_pad), screen_bounds.br() - Point(w_pad, h_pad));
    }

    input_frame.copyTo(output_frame, mask);
    output_frame = output_frame(padded_screen_bounds);

    extract_value(output_frame, output_frame);
    threshold(output_frame, output_frame, 127, 255, THRESH_BINARY);

    // imshow("tmp", frame_contours);

    // Canny(blurred, edges, 60, 180);

    // Mat contour_frame(resized.size(), CV_8UC3, Scalar(0, 0, 0));

    // vector<vector<Point>> contours_poly(contours.size());
    // vector<Point> screen_contour;
    // Rect screen_bounds;

    // Mat debug = blurred;

    // for (size_t idx = 0; idx < 25; idx++)
    // {
    //     int acc = 0.018 * arcLength(contours[idx], true);
    //     approxPolyDP(Mat(contours[idx]), contours_poly[idx], acc, true);

    //     if (contours_poly[idx].size() == 4)
    //     {
    //         screen_contour = contours_poly[idx];
    //         screen_bounds = boundingRect(contours_poly[idx]);
    //         rectangle(debug, screen_bounds, Scalar(0), 2);
    //         cout << screen_bounds << endl;
    //         // break;
    //     }
    // }

    // imshow("tmp", edges);

    // int w_pad = screen_bounds.width * 0.05;
    // int h_pad = screen_bounds.height * 0.05;
    // Rect padded_screen_bounds = Rect(screen_bounds.tl() + Point(w_pad, h_pad), screen_bounds.br() - Point(w_pad, h_pad));

    // input_frame = Scalar::all(255) - input_frame;
    // color_reduce(input_frame);
    // input_frame.copyTo(output_frame, mask);
}

int main(int argc, char **argv)
{
    string window_name = "Webcam";
    Mat frame;

    namedWindow(window_name, WINDOW_NORMAL);
    // namedWindow("tmp", WINDOW_NORMAL);
    // namedWindow("tmp2", WINDOW_NORMAL);

    frame = imread(image_path, IMREAD_COLOR);

    Mat processed_frame;
    process_frame(frame, processed_frame);

    if (!processed_frame.empty())
    {
        tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();

        ocr->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
        ocr->SetPageSegMode(tesseract::PSM_SINGLE_LINE);

        ocr->SetImage(processed_frame.data, processed_frame.cols, processed_frame.rows, 3, processed_frame.step);
        string licence_plate = string(ocr->GetUTF8Text());
        cout << licence_plate << endl;

        ocr->End();

        imshow(window_name, processed_frame);
    }

    waitKey(0);

    // while (true)
    // {
    //     cap >> frame;

    //     Mat processed_frame;
    //     process_frame(frame, processed_frame);

    // tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();

    // ocr->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
    // ocr->SetPageSegMode(tesseract::PSM_SINGLE_LINE);

    // ocr->SetImage(processed_frame.data, processed_frame.cols, processed_frame.rows, 3, processed_frame.step);
    // string licence_plate = string(ocr->GetUTF8Text());
    // cout << licence_plate << endl;

    // ocr->End();

    //     imshow(window_name, frame);

    //     if (waitKey(25) == 113)
    //         break;
    // }

    return 0;
}
