#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

using namespace cv;
using namespace std;

const int SCALING_FACTOR = pow(2, 16);

#define H_MASK  0xFFFF0000
#define L_MASK  0xFFFF
#define HH_MASK 0xFFFFFFFF00000000

#define H_BITS 16
#define L_BITS 16

float to_float(uint f)
{
    float res;
    float rh = (f & H_MASK) / SCALING_FACTOR;
    float rl = (float)(f & L_MASK)/ SCALING_FACTOR;
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
    uint mh  = (m & HH_MASK) >> H_BITS;
    uint ml  = (m & H_MASK)  >> L_BITS;
    uint res =  mh + ml;

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


Mat horizontal(Mat &src, uint dRows, uint dCols, uint fy, uint fx)
{
    Mat dst = Mat::zeros(dRows, dCols, CV_8UC1);
    uint tx, ty;
    uint fact_x, fact_y;
    uint iX, iXN, iY, iYN;
    uint res, rt, rb;
    uint first, second;

    uint one = to_fixed(1);

    for(uint row = 0; row < dRows; row++)
    {
        for(uint col = 0; col < dCols; col++)
        {
            tx = mul(to_fixed(col), fx);
            ty = mul(to_fixed(row), fy);

            iX = tx & H_MASK;
            iXN = iX + one;

            iY = ty & H_MASK;
            iYN = iY + one;

            fact_x = tx & L_MASK;
            fact_y = ty & L_MASK;

            if(iX >= to_fixed(dCols)) {
                iX = to_fixed(dCols)-one;
            }

            if(iXN >= to_fixed(dCols)) {
                iXN = to_fixed(dCols)-one;
            }

            if(iY >= to_fixed(dRows)) {
                iY = to_fixed(dRows)-one;
            }

            if(iY >= to_fixed(dRows)) {
                iYN = to_fixed(dRows)-one;
            }

            int a = src.at<uchar>(iY  >>16, iX  >>16, 0);
            int b = src.at<uchar>(iY  >>16, iXN >>16, 0);
            int c = src.at<uchar>(iYN >>16, iX  >>16, 0);
            int d = src.at<uchar>(iYN >>16, iXN >>16, 0);

            rt = mul((one-fact_x), to_fixed(a))+mul(fact_x,to_fixed(b));
            rb = mul((one-fact_x), to_fixed(c))+mul(fact_x,to_fixed(d));

            first  = mul((one-fact_y), rt);
            second = mul(fact_y, rb);
            res = first + second;

            // res = mul((one-fact_y), to_fixed(rb))+mul(fact_y, to_fixed(rt));

            if(col == -15)
            {
                cout << "tx: "; print_fixed(tx);
                cout << "ty: "; print_fixed(ty);
                cout << endl;
                cout << "fact_x  : "; print_fixed(fact_x);
                cout << "fact_y  : "; print_fixed(fact_y);
                cout << endl;
                cout << "iX: "; print_fixed(iX);
                cout << "iXN: "; print_fixed(iXN);
                cout << endl;
                cout << "iY: "; print_fixed(iY);
                cout << "iYN: "; print_fixed(iYN);
                cout << endl;
                cout << "ABCD vals: " << a << " " << " " << b << " " << c << " " << d << endl;
                cout << "RT:  "; print_fixed(rt);
                cout << "RB:  "; print_fixed(rb);
                cout << "f    "; print_fixed(first);
                cout << "s    "; print_fixed(second);
                cout << "Res: "; print_fixed(res);
                cout << "#####################" << endl;
            }

            dst.at<uchar>(row, col, 0) = (res & H_MASK)>>16;
        }
    }

    return dst;
}

int main(int argc, char** argv )
{
    Mat image, dst;
    image = imread("compiling.png", CV_LOAD_IMAGE_GRAYSCALE );

    uint dRows = 640;
    uint dCols = 480;

    float fx  = (float)image.cols/dCols;
    uint  ufx = to_fixed(fx);

    float fy  = (float)image.rows/dRows;
    uint  ufy = to_fixed(fy);

    dst = horizontal(image, dRows, dCols, ufy, ufx);
    imshow("cv image", dst);
    while(waitKey(0) != 27);

    return 0;
}
