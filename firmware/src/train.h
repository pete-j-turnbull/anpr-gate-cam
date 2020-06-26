#ifndef TRAIN_H
#define TRAIN_H

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/ml.hpp>

#include "feature.h"
#include "utils.h"
#include <iostream>

using namespace cv;
using namespace std;
using namespace cv::ml;

bool train(string save_path, string train_path)
{
    const int number_of_class = 35;
    const int number_of_sample = 1;
    const int number_of_feature = 32;

    Ptr<SVM> svm = SVM::create();
    svm->setType(SVM::C_SVC);
    svm->setKernel(SVM::LINEAR);
    svm->setGamma(0.5);
    svm->setC(16);
    svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6));

    vector<string> folders = list_folder(train_path);

    if (folders.size() <= 0)
    {
        return false;
    }

    if (number_of_class != folders.size() || number_of_sample <= 0 || number_of_class <= 0)
    {
        return false;
    }

    Mat src;
    Mat data = Mat(number_of_sample * number_of_class * 6, number_of_feature, CV_32FC1);
    Mat label = Mat(number_of_sample * number_of_class * 6, 1, CV_32SC1);

    int index = 0;
    sort(folders.begin(), folders.end());

    for (size_t i = 0; i < folders.size(); ++i)
    {
        vector<string> files = list_file(folders.at(i));

        if (files.size() <= 0 || files.size() != number_of_sample)
            return false;

        string folder_path = folders.at(i);
        cout << "list folder" << folders.at(i) << endl;

        string label_folder = folder_path.substr(folder_path.length() - 1);
        for (size_t j = 0; j < files.size(); ++j)
        {
            src = imread(files.at(j));
            if (src.empty())
            {
                return false;
            }

            vector<float> feature = calculate_feature(src);
            for (size_t t = 0; t < feature.size(); ++t)
            {
                data.at<float>(index * 6, t) = feature.at(t);
                data.at<float>(index * 6 + 1, t) = feature.at(t);
                data.at<float>(index * 6 + 2, t) = feature.at(t);
                data.at<float>(index * 6 + 3, t) = feature.at(t);
                data.at<float>(index * 6 + 4, t) = feature.at(t);
                data.at<float>(index * 6 + 5, t) = feature.at(t);
            }

            label.at<int>(index * 6, 0) = i;
            label.at<int>(index * 6 + 1, 0) = i;
            label.at<int>(index * 6 + 2, 0) = i;
            label.at<int>(index * 6 + 3, 0) = i;
            label.at<int>(index * 6 + 4, 0) = i;
            label.at<int>(index * 6 + 5, 0) = i;

            index++;
        }
    }

    // SVM Train OpenCV 3.1
    svm->trainAuto(ml::TrainData::create(data, ml::ROW_SAMPLE, label));
    svm->save(save_path);

    return true;
}

#endif