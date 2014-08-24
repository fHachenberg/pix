#include "pix.h"

#include <fstream>
#include <iostream>

#include <string>
#include <vector>

#include <tclap/CmdLine.h>

const char* pix_cmline_version = "1.0";

int main(int argc, char* argv[]) {
    //See commandline argument descriptions for the meaning and defaults of the following option variables
    //Input spec
    std::string inputfile;
    bool use_alpha; 
    //Algorithm main args
    int target_width;
    int target_height;
    int target_numcolors;
    //Output spec
    bool show; 
    std::string outputfile;
    //Algorithm fine-tuning
    int slic_factor;
    float saturation;
    float smooth_factor;
    float sigma_color;
    float sigma_position;
    
    int max_iter;
    
    try {
        TCLAP::CmdLine cmd("Scales down resolution and color palette size of an image, using Timothy Gerstner's PIX algorithm.", ' ', pix_cmline_version);
        TCLAP::UnlabeledValueArg<std::string> input_arg("in", "input filename", true, "", "input filename");
        TCLAP::UnlabeledValueArg<int> target_width_arg("width", "Output width. You can specify 0 to let the width be determined by the height and reproducing the width:height ratio of the input image", true, 0, "width");
        TCLAP::UnlabeledValueArg<int> target_height_arg("height", "Output height. You can specify 0 to let the height be determined by the width and reproducing the width:height ratio of the input image", true, 0, "height");
        TCLAP::UnlabeledValueArg<int> target_numcolors_arg("numcolors", "Output number colors", true, 0, "number colors");
        TCLAP::ValueArg<std::string> output_arg("o", "out", "output filename", false, "", "filename");
        TCLAP::ValueArg<int> maxiter_arg("m", "max-iterations", "Maximum number of iterations", false, 128, "number iterations");
        
        TCLAP::ValueArg<int> slic_factor_arg("l", "slic-factor", "SLIC factor", false, 45, "integer value");
        TCLAP::ValueArg<float> saturation_arg("t", "saturation", "Saturation value", false, 1.1f, "floating point value");
        TCLAP::ValueArg<float> smooth_factor_arg("f", "smooth-factor", "Smooth factor", false, 0.1f, "floating point value");
        TCLAP::ValueArg<float> sigma_color_arg("c", "sigma", "Sigma value (color)", false, 2.0f, "floating point value");        
        TCLAP::ValueArg<float> sigma_position_arg("p", "sigmap", "Sigma value (position)", false, 0.97f, "floating point value");
        
        TCLAP::SwitchArg use_alpha_arg("a","use-alpha","Use Alpha-Channel for Importance Sampling", false);
        TCLAP::SwitchArg show_arg("s","show","Show the result in a modal dialogue", false);
        cmd.add(input_arg);
        cmd.add(target_width_arg);
        cmd.add(target_height_arg);
        cmd.add(target_numcolors_arg);
        cmd.add(use_alpha_arg);
        cmd.add(show_arg);
        cmd.add(output_arg);
        cmd.add(maxiter_arg);
        cmd.add(slic_factor_arg);
        cmd.add(saturation_arg);
        cmd.add(smooth_factor_arg);
        cmd.add(sigma_color_arg);
        cmd.add(sigma_position_arg);
        
        cmd.parse( argc, argv );
        
        inputfile = input_arg.getValue();        
        target_width = target_width_arg.getValue();
        target_height = target_height_arg.getValue();
        if(target_width == 0 && target_height == 0) {
            std::cerr << "You cannot specify 0 for both width and height" << std::endl;
            return 1;
        }        
        target_numcolors = target_numcolors_arg.getValue();
        use_alpha = use_alpha_arg.getValue();      
        outputfile = output_arg.getValue(); 
        show = show_arg.getValue(); 
        max_iter = maxiter_arg.getValue();
        slic_factor = slic_factor_arg.getValue();
        saturation = saturation_arg.getValue();
        smooth_factor = smooth_factor_arg.getValue();
        sigma_color = sigma_color_arg.getValue();
        sigma_position = sigma_position_arg.getValue();
    }
    // catch any cmdline exceptions
    catch (TCLAP::ArgException &e) { 
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }    
        
    cv::Mat imagei;
    //reads Image as 3 CV_8UC3
    imagei = cv::imread(inputfile, CV_LOAD_IMAGE_COLOR);
    if(!imagei.data)
    {
        std::cerr << "Could not open or find the image" << std::endl;
        return 1;
    }
    
    //Pix expected an image in CV_32FC3 format
    cv::Mat image(cv::Size(imagei.cols, imagei.rows), CV_32FC3);
    imagei.convertTo(image, CV_32FC3, 1/255.0);   
    
    //Invalid case of width == height == 0 is managed above
    if(target_width == 0)
        target_width = target_height * (static_cast<double>(imagei.cols) / imagei.rows);
    else if (target_height == 0)
         target_height = target_width * (static_cast<double>(imagei.rows) / imagei.cols);
    
    Pix pix(image, target_width, target_height, target_numcolors);    
    
    if(use_alpha) {        
        cv::Mat image_full;
        image_full = cv::imread(inputfile, -1); //-1 loads the image as is (including alpha channel)
        if(image_full.channels() != 4) {
            std::cerr << "Trying to use alpha channel as importance map, but image has no alpha channel" << std::endl;
            return 1;
        }
        cv::Mat alpha_int = cv::Mat(cv::Size(image.cols, image.rows), CV_8UC1, cv::Scalar(255));
        CvMat tmp = image_full;
        cv::extractImageCOI(&tmp, alpha_int, 3); //extract alpha channel
        cv::Mat input_weights = cv::Mat(cv::Size(image.cols, image.rows), 
                                CV_32FC1, cv::Scalar(1.0f));
        alpha_int.convertTo(alpha_int, CV_32FC1, 1/255.0);
        
        pix.set_input_weights(input_weights);
    }
        
    pix.SetBilateralParams(sigma_color, sigma_position);
    pix.set_laplacian_factor(smooth_factor);
    pix.setSlicFact(slic_factor);    
    pix.SetSaturation(saturation);    
    
    pix.Initialize();
        
    int num_iterations = 0;
    while(!pix.hasConverged() && num_iterations < max_iter)
    {        
        num_iterations += 1;
        std::cout << ".";  
        std::cout.flush();              
        pix.Iterate();        
        pix.SaveState();        
    }
    std::cout << std::endl;
        
    cv::Mat result;
    pix.GetOutputImage(result);  //GetRegionImage //GetSuperpixelImage //GetOutputImage
    cv::Mat result_big(cv::Size(result.cols*4, result.rows*4), CV_8UC3);
    cv:resize(result, result_big, cv::Size(result.cols*4, result.rows*4), 0, 0, CV_INTER_NN);
    
    if(show) {
        cv::imshow( "Display window", result_big);
        cv::waitKey(0);  
    }
    
    cv::Mat output(cv::Size(target_width, target_height), CV_32FC3);
    pix.GetOutputImage(output);    
    imwrite(outputfile, output);
    
    return 0;
}
