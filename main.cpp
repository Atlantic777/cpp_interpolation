#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

using namespace cv;
using namespace std;

const int SCALING_FACTOR = pow(2, 16);

uint mul(uint a, uint b)
{
    uint ah = a >> 16;
    uint bh = b >> 16;

    uint al = a & 0xFFFF;
    uint bl = b & 0xFFFF;

    int h = (ah*bh);
    int l = (al*bl) >> 16;

    uint res = (h & 0xFFFF0000)  + l;

    return res;
}

uint to_fixed(float f)
{
    uint h = floor(f)*SCALING_FACTOR;
    uint l = (uint)((f-h)*SCALING_FACTOR);

    return h + l;
}

float to_float(uint f)
{
    float res;
    float rh = f /SCALING_FACTOR;
    float rl = (float)(f & 0xFFFF)/SCALING_FACTOR;
    res = rh + rl;
    return res;
}

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

    uint f;

    float fx = (float)image.cols/dCols;

    f = floor(fx)*SCALING_FACTOR;
    f = (fx - f)*SCALING_FACTOR;

    float a = 0.5;
    float b = 0.5;

    uint fa = to_fixed(a);
    uint fb = to_fixed(b);

    uint r  = mul(fa, fb);

    // dst = horizontal(image, dRows, dCols, fx);
    // imshow("cv image", dst);
    //
    // while(waitKey(0) != 27);

    return 0;
}
