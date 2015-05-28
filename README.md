Fixed point and bilinear interpolation demo
===========================================

Description:
-----------

This file shows an implementation of fixed point arithmetics and
bilinear interpolation for image enlaring.

Fixed point representation is like following:

- fixed point type is 32 bits long
- lower 16 bits are remainder part
- upper 16 bits are whole part
- you need support for 64 bits integer multiplication

Dependencies:
------------

- OpenCV
- Compiles with CMake using g++ (GCC) on a Linux machine


Compiling
---------

```
sudo apt-get install libopencv-dev
cmake .
make
```
