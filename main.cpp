#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

using namespace cv;
using namespace std;

Mat horizontal(Mat &src, int dRows, int dCols, float fx)
{
    Mat dst = Mat::zeros(dRows, dCols, CV_8UC1);
    float target;
    float fact;
    int iA, iB, res;

    for(int row = 0; row < dRows; row++)
    {
        for(int col = 0; col < dCols; col++)
        {

            target = col*fx;
            iA = floor(target);
            iB = iA + 1;
            fact = target - iA;

            if(iA >= dCols) {
                iA = dCols-1;
            }

            if(iB >= dCols) {
                iB = dCols-1;
            }

            int a = src.at<uchar>(row, iA, 0);
            int b = src.at<uchar>(row, iB, 0);

            res = (1-fact)*a + fact*b;

            dst.at<uchar>(row, col, 0) = res;
        }
    }

    return dst;
}

int main(int argc, char** argv )
{
    Mat image, dst;
    image = imread("compiling.png", CV_LOAD_IMAGE_GRAYSCALE );

    int dRows = image.rows;
    int dCols = 800;

    float fx = (float)image.cols/dCols;
    dst = horizontal(image, dRows, dCols, fx);
    imshow("cv image", dst);


    while(waitKey(0) != 27);

    return 0;
}
