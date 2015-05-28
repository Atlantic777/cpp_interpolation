/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Nikola Hardi - Atlantic777
 * Date  : May, 2015.
 *
 * Description:
 * ===========
 *
 * This file shows an implementation of fixed point arithmetics and
 * bilinear interpolation for image enlaring.
 *
 * Fixed point representation is like following:
 *
 *  - fixed point type is 32 bits long
 *  - lower 16 bits are remainder part
 *  - upper 16 bits are whole part
 *  - you need support for 64 bits integer multiplication
 *
 * Dependencies:
 * ============
 *
 *  - OpenCV
 *  - Compiles with CMake using g++ (GCC) on a Linux machine
 */
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

/* get fixed point number - return a float
 * ---------------------------------------
 *  Firstly, we get upper 16 bits of fixed point datatype by mask H_MASK
 *  and divide it by the SCALING_FACTOR. Effectively, it's the same
 *  as shift + cast to float.
 *
 *  After that, we get lower 16 bits with L_MASK and divide that with
 *  SCALING_FACTOR. This time, we expect to get numbers smaller than 1
 *  so we need to use "float dividing". It is done by casting one operand to
 *  float and then doing the division.
 *
 *  This code should run on a machine which has floating point arithmetics
 *  implemented. This is usefull for reinterpeting results of results
 *  represented in fixed point.
 *
 */
float to_float(uint f)
{
    float rh =        (f & H_MASK)/SCALING_FACTOR;
    float rl = (float)(f & L_MASK)/SCALING_FACTOR;

    return rh + rl;
}

/* Just a utility function for printing fixed point data.
 * It uses to_float() and prints to stdout.
 */
void print_fixed(uint a)
{
    cout << to_float(a) << endl;
}

/* Multiply two fixed point nubmers
 * --------------------------------
 *  Fixed point multiplication can be a bit tricky. The thing is that
 *  when we multiply two 32 bits numbers, we get 64 bits long result which
 *  we can't store and use further so we need to switch it back to regular
 *  fixed point format we use.
 *
 *  Firstly, we do regular integer multiplication but we have to keep in mind
 *  that result is two times as wide as operands are so we have to cast at least
 *  one operand to long.
 *
 *  After that, we get upper 32 bits and store them as upper part.
 *  This represents "whole part" of non-integer number.
 *
 *  We do the same for lower 32 bits of multiplication product,
 *  and this represents "remainder part" of non-integer number.
 *
 *  Shifting and byte slicing is done implicitly in some way.
 *  When you shift 64 bit data by 16 bits and store it to 32 bit datatype,
 *  only first 16 bits of upper 32 bits will be stored in destination
 *  which is 32 bits wide, too.
 *
 *  - original data
 *  0x 1111 2222 3333 4444 || 5555 6666 7777 8888
 *
 *  - masked with HH_MASK
 *  0x 1111 2222 3333 4444 || 0000 0000 0000 0000
 *
 *  - shifted by 16 bits (2 bytes)
 *  0x 0000 0000 1111 2222 || 3333 4444 0000 0000
 *
 *  - we are storing this into uint (32 bits) destination
 *  0x 3333 4444 0000 00000
 *
 *  Now, we have "whole part" in upper 16 bits and lower 16 bits are free
 *  so we can store "remainder part" there.
 *
 *  For remainder part it's a bit easier to explain. Just use H_MASK
 *  to get upper 16 bits of lower 32 bits part and shift that to fit
 *  into lower 16 bits of 32 bits destination datatype. That's the place
 *  we have left free in the "whole part".
 *
 *  It's more intuitive to use something like HL_MASK, which would leave
 *  whole lower part of 64 bit result, that means lower 32 bits and then
 *  shift it, but as we are losing those lower 16 bits with shifting,
 *  it doesn't change anything. I already wrote longer comment than that
 *  mask would be, so I probably should have just done that.
 *
 *  We can safely sum those two parts of result. It's like 2.0 + 0.5 = 2.5
 */
uint mul(uint a, uint b)
{
    unsigned long m = (unsigned long)a*b;
    uint mh  = (m & HH_MASK) >> H_BITS;
    uint ml  = (m & H_MASK)  >> L_BITS;
    uint res =  mh + ml;

    return res;
}

/* get a float - return fixed point number
 * ---------------------------------------
 *  Result of this function can then be used for calculations
 *  in pure fixed point on hardware without a FPU.
 *
 *  This is usefull for simulating division in fixed point software by
 *  multiplying with reciprocal value which is calculated on a full blown PC.
 *
 *  Use this function for transition from floating point capable hardware
 *  to the fixed point hardware.
 *
 *  Get "whole part", scale it to fit into upper 16 bits of fixed point
 *  datatype and then convert it from float to integer representation.
 *  It could be done other way round if you get whole part, cast it to int
 *  and then use plain shifting.
 *
 *  NOTE: you can't shift floating point numbers!
 *
 *  For "remainder part" just substitute "whole part" from input float,
 *  scale it up and then cast it to uint.
 */
uint to_fixed(float f)
{
    uint h = floor(f)*SCALING_FACTOR;
    uint l = (uint)((f-floor(f))*SCALING_FACTOR);

    return h + l;
}

/* scale up image
 *   src   - OpenCV Mat with original image
 *   dRows - number of destination rows (not fixed point)
 *   dCols - number of destination cols (not fixed point)
 *   fy    - horizontal scaling factor (fixed point)
 *   fx    - vertical   scaling factor (fixed point)
 *   ------------------------------------------------------
 *  This function implmeents bilionear interpolation
 *  in pure fixed point arithmetics.
 *
 *  Firstly, it alocates space for destination image which is also
 *  returned as return value of this function.
 *
 *  It iterates through every destination pixel, gets nearest source pixels
 *  and interpolates value. Here's how it's done.
 *
 *  Firstly, we need to "project" destination pixel onto source image.
 *  We do this by mupltiplying current pixel row and column by fy and fx
 *  factors.
 *
 *  After that, we get whole part of that numbers by leaving only
 *  upper 16 bits, remainder part by leaving only lower 16 bits and neighours
 *  by adding 1 (in fixed point) to the resulting positions we calculated.
 *
 *  Keep in mind that you can sum two fixed point numbers as you would
 *  regular integers but those two numbers _have to be both_ in fixed point.
 *  This is why we have defined fOne which is simple integer 1,
 *  but in fixed point representation.
 *
 *  We need to check boundaries of source images. If we have stepped out
 *  of boundaries, just limit it to max values.
 *
 *  When we have correct positions for neighbour pixels in source image,
 *  we get values of that pixels. Keep in mind that our positions are
 *  in fixed point, and source image pixel positions are not in fixed point
 *  so shifting is necessarry to extract whole parts and move back to
 *  regular integer representation.
 *
 *  When pixel values are extracted, we can move to actual interpolation.
 *  It's as simple as: (1-f)*A + f*B.
 *
 *  Here's how it looks actually:
 *    - Points A, B, C and D are source pixels
 *    - res is our target pixel
 *    - rt and rb are temporary top and bottom results
 *    - firstly we get rt and rb and then get res
 *    - repeat this for every destination pixel
 *
 *  NOTE: keep in mind that pixel values are in 8 bit unsigned integers.
 *  That means that you have to extract whole part of fixed point number
 *  and convert it to regular int representation by shifting.
 *
 *   (1-f)     f
 *  A --- rt ------- B
 *  |      | (1-f)   |
 *  |      |         |
 *  |     res        |
 *  |      |         |
 *  |      | f       |
 *  |      |         |
 *  C --- rb ------- D
 */
Mat interpolate(Mat &src, uint dRows, uint dCols, uint fy, uint fx)
{
    Mat dst = Mat::zeros(dRows, dCols, CV_8UC1);

    uint tx, ty;
    uint fact_x, fact_y;
    uint iX, iXN, iY, iYN;
    uint res, rt, rb;
    uint first, second;

    uint fOne = to_fixed(1);

    uint fsColsLim = to_fixed(src.cols) - fOne;
    uint fsRowsLim = to_fixed(src.rows) - fOne;

    for(uint row = 0; row < dRows; row++)
    {
        for(uint col = 0; col < dCols; col++)
        {
            tx = mul(to_fixed(col), fx);
            ty = mul(to_fixed(row), fy);

            iX  = tx & H_MASK;
            iXN = iX + fOne;

            iY  = ty & H_MASK;
            iYN = iY + fOne;

            fact_x = tx & L_MASK;
            fact_y = ty & L_MASK;

            if(iX > fsColsLim) {
                iX = fsColsLim;
            }

            if(iXN > fsColsLim) {
                iXN = fsColsLim;
            }

            if(iY > fsRowsLim) {
                iY = fsRowsLim;
            }

            if(iY > fsRowsLim) {
                iYN = fsRowsLim;
            }

            int a = src.at<uchar>(iY  >>16, iX  >>16, 0);
            int b = src.at<uchar>(iY  >>16, iXN >>16, 0);
            int c = src.at<uchar>(iYN >>16, iX  >>16, 0);
            int d = src.at<uchar>(iYN >>16, iXN >>16, 0);

            rt = mul((fOne-fact_x), to_fixed(a))+mul(fact_x,to_fixed(b));
            rb = mul((fOne-fact_x), to_fixed(c))+mul(fact_x,to_fixed(d));

            first  = mul((fOne-fact_y), rt);
            second = mul(fact_y, rb);
            res = first + second;

            dst.at<uchar>(row, col, 0) = (res & H_MASK)>>16;
        }
    }

    return dst;
}

int main(int argc, char** argv )
{
    // Load image
    Mat image, dst;
    image = imread("compiling.png", CV_LOAD_IMAGE_GRAYSCALE );

    // Define output dimensions
    uint dRows = 640;
    uint dCols = 480;

    // Get fx and fy scaling factors in fixed point
    float fx  = (float)image.cols/dCols;
    uint  ufx = to_fixed(fx);

    float fy  = (float)image.rows/dRows;
    uint  ufy = to_fixed(fy);

    // Do interpolation
    dst = interpolate(image, dRows, dCols, ufy, ufx);

    // Show result (exit with ESC - key 27)
    imshow("cv image", dst);
    while(waitKey(0) != 27);

    return 0;
}
