#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <iostream>

#include "./src/train.h"
#include "./src/recog.h"

using namespace std;

void run_train()
{
    bool success = train("svm.txt", "./data");
    if (success)
        cout << "SVM Training Completed" << endl;
    else
        cout << "SVM Training Failed" << endl;
}

void run_predict()
{
    string img_path = "./test/IMG_0077.jpg";
    Mat img = imread(img_path, IMREAD_COLOR);

    vector<string> plates = predict(img);

    for (size_t i = 0; i < plates.size(); ++i)
    {
        cout << plates[i] << endl;
    }
}

// this needs to read video stream and check once per second for plates
// then it needs to tell a webserver that the plate is present and ask if it can open

// display green if allowed, red if not and spinning white while checking.
// Once movement is detected start analyzing frames, if after 5 seconds we haven't found an allowed plate stop set red
//

// we need a separate program to respond to gate controls - this includes the automatic opening from plate analysis

int main(int argc, char *argv[])
{
    run_predict();
    // run_train();

    if (waitKey(0))
        return 0;
}
