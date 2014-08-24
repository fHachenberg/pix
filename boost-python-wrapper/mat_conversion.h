# ifndef __COVERSION_OPENCV_H__
# define __COVERSION_OPENCV_H__

/*
 * The following conversion functions are taken/adapted from OpenCV's cv2.cpp file
 * inside modules/python/src2 folder.
 * See https://github.com/yati-sagade/blog-content/blob/master/content/numpy-boost-python-opencv.rst
 */
 
#include <Python.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "numpy/ndarrayobject.h"

static PyObject* opencv_error = 0;

static int failmsg(const char *fmt, ...);

class PyAllowThreads;

class PyEnsureGIL;

#define ERRWRAP2(expr) \
try \
{ \
    PyAllowThreads allowThreads; \
    expr; \
} \
catch (const cv::Exception &e) \
{ \
    PyErr_SetString(opencv_error, e.what()); \
    return 0; \
}

static PyObject* failmsgp(const char *fmt, ...);

//Offset of ref counter inside Numpy-Array
//Is this an invariant throughout Numpy versions?
static size_t REFCOUNT_OFFSET = (size_t)&(((PyObject*)0)->ob_refcnt) +
    (0x12345678 != *(const size_t*)"\x78\x56\x34\x12\0\0\0\0\0")*sizeof(int);

static inline PyObject* pyObjectFromRefcount(const int* refcount)
{
    return (PyObject*)((size_t)refcount - REFCOUNT_OFFSET);
}

static inline int* refcountFromPyObject(const PyObject* obj)
{
    return (int*)((size_t)obj + REFCOUNT_OFFSET);
}

//Creates a cv::Mat object which is just a frontend for a NumPy-NDArray
cv::Mat createNumpyArrayWithMatWrapper(int rows, int cols, int type);

class NumpyAllocator;

enum { ARG_NONE = 0, ARG_MAT = 1, ARG_SCALAR = 2 };

/* This class is used to create a cv:Mat object wrapping an existing
 * NumPy NDArray (toMat). The underlying NumPy array can be extracted
 * using toNDArray.

 */
class NDArrayConverter
{
private:
    void init();
public:
    NDArrayConverter();
    
    cv::Mat* toMat(const PyObject *o, void *memory_chunk);
    PyObject* toNDArray(const cv::Mat& mat);
};

# endif
