#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <tesseract/baseapi.h>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;
using namespace cv;

string image_path = "/Users/pete/Development/iot/anpr-gate-cam/firmware/plate1.jpg";

bool compareContourAreas(vector<Point> contour1, vector<Point> contour2)
{
    double i = fabs(contourArea(Mat(contour1)));
    double j = fabs(contourArea(Mat(contour2)));
    return (i > j);
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
    Mat blurred, thresh, edges, dilated;

    medianBlur(input_frame, blurred, 15);
    threshold(blurred, thresh, 127, 255, THRESH_BINARY);

    Canny(thresh, edges, 50, 200, 3);

    int dilation_size = 3;
    Mat element = getStructuringElement(MORPH_RECT,
                                        Size(2 * dilation_size + 1, 2 * dilation_size + 1),
                                        Point(dilation_size, dilation_size));
    dilate(edges, dilated, element);

    vector<vector<Point>> contours;
    findContours(dilated, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
    sort(contours.begin(), contours.end(), compareContourAreas);

    vector<vector<Point>> contours_poly(contours.size());
    vector<Point> screen_contour;
    Rect screen_bounds;

    Mat frame_contours(dilated.size(), CV_8UC1, Scalar(0));
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
                cout << contours_poly[i] << endl;
                screen_bounds = boundingRect(contours_poly[i]);

                drawContours(input_frame, contours_poly, i, Scalar(255), 2);
                // break;
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

        input_frame.copyTo(output_frame, mask);
        output_frame = output_frame(padded_screen_bounds);

        extract_value(output_frame, output_frame);
        threshold(output_frame, output_frame, 127, 255, THRESH_BINARY);
    }
}

int main(int argc, char **argv)
{
    Mat frame;

    namedWindow("Result", WINDOW_NORMAL);
    namedWindow("tmp", WINDOW_NORMAL);

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

        imshow("Result", frame);
    }

    waitKey(0);

    return 0;
}
