#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

using namespace cv;
using namespace std;

const int SCALING_FACTOR = pow(2, 16);

float to_float(uint f)
{
    float res;
    float rh = (f & 0xFFFF0000) / SCALING_FACTOR;
    float rl = (float)(f & 0xFFFF)/ SCALING_FACTOR;
    res = rh + rl;

    return res;
}

void print_fixed(uint a)
{
    cout << to_float(a) << endl;
}

uint mul(uint a, uint b)
{
    unsigned long m = (unsigned long)a*b;
    uint mh = (m&0xFFFFFFFF00000000) >> 16;
    uint ml = (m&0xFFFF0000) >> 16;
    uint res = mh + ml;

    // cout << "beg mul" << endl;
    // cout << m << endl;
    // cout << "op1: "; print_fixed(a);
    // cout << "op2: "; print_fixed(b);
    // cout << "mh : "; print_fixed(mh);
    // cout << "ml : "; print_fixed(ml);
    // cout << "end mul\n" << endl;

    return res;
}

uint to_fixed(float f)
{
    uint h = floor(f)*SCALING_FACTOR;
    uint l = (uint)((f-floor(f))*SCALING_FACTOR);

    return h + l;
}


Mat horizontal(Mat &src, uint dRows, uint dCols, uint fx)
{
    Mat dst = Mat::zeros(dRows, dCols, CV_8UC1);
    uint target;
    uint fact;
    uint iA, iB, res;

    uint one = to_fixed(1);

    for(uint row = 0; row < dRows; row++)
    {
        for(uint col = 0; col < dCols; col++)
        {
            target = mul(to_fixed(col),fx);
            iA = target & 0xFFFF0000;
            iB = iA + one;
            fact = target & 0xFFFF;

            if(col > 3 && col < 7 && row == 0)
            {
                cout << "target: "; print_fixed(target);
                cout << "fact  : "; print_fixed(fact);
                cout << "iA: "; print_fixed(iA);
                cout << "iB: "; print_fixed(iB);
                cout << endl;
            }



            if(iA >= to_fixed(dCols)) {
                iA = to_fixed(dCols)-one;
                cout << "fixing: " << col << endl;
            }

            if(iB >= to_fixed(dCols)) {
                iB = to_fixed(dCols)-one;
                cout << "fixing: " << col << endl;
            }

            int a = src.at<uchar>(row, iA>>16, 0);
            int b = src.at<uchar>(row, iB>>16, 0);

            res = mul((one-fact), to_fixed(a))+mul(fact,to_fixed(b));

            dst.at<uchar>(row, col, 0) = (res&0xFFFF0000)>>16;
        }
    }

    return dst;
}

int main(int argc, char** argv )
{
    Mat image, dst;
    image = imread("compiling.png", CV_LOAD_IMAGE_GRAYSCALE );

    uint dRows = image.rows;
    uint dCols = 1768;

    float fx = (float)image.cols/dCols;
    uint  f  = to_fixed(fx);


    // uint a = to_fixed(0.5);
    // uint b = to_fixed(2.5);
    // print_fixed(a);
    // print_fixed(b);
    // print_fixed(mul(a, b));

    dst = horizontal(image, dRows, dCols, f);
    horizontal(image, dRows, dCols, f);
    imshow("cv image", dst);
    while(waitKey(0) != 27);

    return 0;
}
