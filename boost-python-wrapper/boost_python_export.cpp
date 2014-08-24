#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/raw_function.hpp>

#include "mat_conversion.h"

using namespace boost::python;

#include "pix.h"

using namespace boost::python;

struct MatFromPyObject
{
    MatFromPyObject() 
    {
        import_array();
        converter::registry::push_back( &convertible,
                                        &construct,
                                        type_id<cv::Mat>());
    }
    
    static void* convertible(PyObject* obj_ptr)
    {
        if (PyArray_Check(obj_ptr)) {
            return obj_ptr;
        } else {
            return NULL;
        }
    }
    
    static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data)
    {
        typedef converter::rvalue_from_python_storage<cv::Mat> storage_t;
        storage_t* the_storage = reinterpret_cast<storage_t*>(data);
        void* memory_chunk = the_storage->storage.bytes;
        object obj(handle<>(borrowed(obj_ptr)));
        NDArrayConverter conv;
        cv::Mat* output = conv.toMat(obj_ptr, memory_chunk);                
        // Use the contents of obj to populate output, e.g. using extract<>
        data->convertible = memory_chunk;
    }
};
struct MatCreator
{    
    static PyObject* convert(cv::Mat const& mat)
    {
        NDArrayConverter conv;
        return conv.toNDArray(mat); 
    }
};

//Wraps the GetOutputImage-Method of Pix object Python-friendly
cv::Mat createNumpyNdArrayFromOutputImage(Pix& pa)
{
    cv::Mat output = createNumpyArrayWithMatWrapper(pa.get_output_width(), pa.get_output_height(), CV_8UC3);
    pa.GetOutputImage(output);
    return output;
}

BOOST_PYTHON_MODULE(pix)
{    
    class_<Pix>("Pix", init<const cv::Mat&, int, int, int>())
        .def(init<std::string>())
        .def("initialize", &Pix::Initialize)
        .def("saveToFile", &Pix::SaveToFile)
        .def("iterate", &Pix::Iterate)
        .add_property("converged", &Pix::hasConverged)
        .add_property("output_image", &createNumpyNdArrayFromOutputImage)
    ;
    
    MatFromPyObject();
    to_python_converter<cv::Mat, MatCreator, false>(); //"true" because tag_to_noddy has member get_pytype
}
