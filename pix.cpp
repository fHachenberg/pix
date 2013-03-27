/* 
Copyright (c) 2013, Timothy Gerstner, All rights reserved.

This code is part of the prototype C++ implementation of our paper/ my thesis.

Public repository: https://github.com/timgerst/pix
Project Webpage:  http://www.research.rutgers.edu/~timgerst/

This code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this code.  If not, see <http://www.gnu.org/licenses/>.

Author: Timothy Gerstner, timgerst@cs.rutgers.edu
*/


#include "pix.h"
#include "const.h"


Pix::Pix(const cv::Mat& img_input, int w, int h, int p) {
	output_width_ = w;
	output_height_ = h;
	max_palette_size_ = p;
	input_width_ = img_input.cols;
	input_height_ = img_input.rows;
	
	input_weights_ = 
    cv::Mat(cv::Size(input_width_, input_height_), CV_32FC1, cv::Scalar(1.0f));
	slic_factor_ = 45;
	smooth_pos_factor_ = .4f;
	sigma_color_	= .87f;
	sigma_position_ = .87f;
	state_list_ = new stateList(kMaxUndo);
	converged_flag_ = false;
	palette_maxed_flag_ = false;
	GetCurrentState()->saturation = 1.1;

	cvtColor(img_input, input_img_,CV_RGB2Lab);
}
Pix::Pix(std::string filename) {
	std::vector<std::string> extensions;
	cv::FileStorage file_storage(filename, cv::FileStorage::READ);
	state_list_ = new stateList(kMaxUndo);
	
	//load orignal image
	file_storage["input_width_"] >> input_width_;
	file_storage["input_height_"] >> input_height_;
	input_img_ = cv::Mat(input_height_,input_width_, CV_32FC3);
	file_storage["input_img_"] >> input_img_;

	//load output size
	file_storage["output_width_"] >> output_width_;
	file_storage["output_height_"] >> output_height_;


	output_img_ = 
    cv::Mat(cv::Size(output_width_,output_height_),CV_32FC3, cv::Scalar(0.0f));
	GetCurrentState()->superpixel_color = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC3, cv::Scalar(0.0f));

	range_ = sqrt((input_height_/(float)output_height_) *
                (input_width_/(float)output_width_));
	
	//load palette
	file_storage["max_palette_size_"] >> max_palette_size_;

  {
	  cv::FileNode node = file_storage["palette"];
	  std::vector<float> in_colors;
	  for(cv::FileNodeIterator it = node.begin(); it != node.end(); ++it) {
		  in_colors.push_back((float)*it);
	  }
	  for(unsigned int i = 0; i< in_colors.size(); i += 3) {
		  GetCurrentState()->palette.push_back(cv::Vec3f(in_colors[i], 
                                                     in_colors[i+1], 
                                                     in_colors[i+2]));
	  }
  }
 

	//load palette_prob
  {
    cv::FileNode node = file_storage["prob_c"];
	  for(cv::FileNodeIterator it = node.begin(); it != node.end(); ++it) {
		  GetCurrentState()->prob_c.push_back((float)*it);
	  }
  }
	//load locked colors
  {
	  cv::FileNode node = file_storage["locked_colors"];
	  GetCurrentState()->locked_colors = 
      std::vector<bool>(max_palette_size_, false); 
	  for(cv::FileNodeIterator it = node.begin(); it != node.end(); ++it) {
		  int next = (int)*it;
		  GetCurrentState()->locked_colors[next] = true;
	  }
  }
	//load superpixel position
	GetCurrentState()->superpixel_pos = 
    cv::Mat(cv::Size(output_width_, output_height_), CV_32FC2);
	file_storage["superpixel_pos"] >> GetCurrentState()->superpixel_pos;

	//load superpixel assignment
	GetCurrentState()->palette_assign = 
    cv::Mat(cv::Size(output_width_,output_height_),CV_32SC1, cv::Scalar(0.0f));
	file_storage["palette_assign"] >> GetCurrentState()->palette_assign;

	//load locked pixels
  {
	  cv::FileNode node = file_storage["pixel_constraints"];
	  GetCurrentState()->pixel_constraints =  
      std::vector<std::list<int> >(output_width_*output_height_);
	  std::list<int> inFixedPix;
	  for(cv::FileNodeIterator it = node.begin(); it != node.end(); ++it) {
		  inFixedPix.push_back((int)*it);
	  }
	  int k = 0;
	  for(std::list<int>::iterator nfp = inFixedPix.begin(); nfp != inFixedPix.end(); ++nfp) {
		  if(*nfp == -1) {
			  k++;
		  } else {
			  GetCurrentState()->pixel_constraints[k].push_back(*nfp);
		  }
	  }
  }

	//load weights
	input_weights_ = cv::Mat(cv::Size(input_width_, input_height_), CV_32FC1);
	file_storage["input_weights_"] >> input_weights_;

	file_storage["iteration"] >> GetCurrentState()->iteration;
	file_storage["slic_factor_"] >> slic_factor_;
	file_storage["sigma_color_"] >> sigma_color_;
	file_storage["sigma_position_"] >> sigma_position_;
	file_storage["smooth_pos_factor_"] >> smooth_pos_factor_;
	file_storage["Saturation"] >> GetCurrentState()->saturation;
  //assume state was saved after initial convergence
	palette_maxed_flag_ = true;
	converged_flag_ = true;
  temperature_ = kTF;

	UpdateSuperpixelMapping();
	UpdateSuperpixelMeans();
	AssociatePalette();

	file_storage.release();
}
Pix::~Pix() {
	delete state_list_;
}
void Pix::Initialize()
{
	
	//find SLIC weighting factor based on expected number of input pixels in a 
  //superpixel
	range_ = sqrt((input_height_/(float)output_height_) *
                (input_width_/(float)output_width_));


	output_img_ = 
    cv::Mat(cv::Size(output_width_,output_height_),CV_32FC3, cv::Scalar(0.0f));
	GetCurrentState()->palette_assign = 
    cv::Mat(cv::Size(output_width_,output_height_),CV_32SC1, cv::Scalar(0.0f));
	GetCurrentState()->iteration = 0;
	

	//Initialize superpixel superpixel positions in a regular grid
	GetCurrentState()->superpixel_pos = 
    cv::Mat(cv::Size(output_width_,output_height_), CV_32FC2);
	for(int y = 0; y < output_height_ ; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			float i = ((x+.5f)/(float)output_width_*input_width_);
			float j = ((y+.5f)/(float)output_height_*input_height_);
			GetCurrentState()->superpixel_pos.at<cv::Vec2f>(y,x) = cv::Vec2f(i,j);
		}
	}
	//assign each input pixel to the closest superpixel in (X,Y) space
	region_map_ = cv::Mat(cv::Size(input_width_, input_height_),CV_32SC2);
	region_list_.clear();
	region_list_ = 
    cv::vector<cv::vector<cv::Vec2i> >(output_width_*output_height_);
	for(int y = 0; y < input_height_; ++y) {
		for(int x = 0; x < input_width_; ++x) {
			int i = (int)( x/(float)input_width_*output_width_ );
			int j = (int)( y/(float)input_height_*output_height_ );
			region_map_.at<cv::Vec2i>(y,x) = cv::Vec2i(i,j);
			region_list_[vec2idx(cv::Vec2i(i,j))].push_back(cv::Vec2i(x,y));
		}
	}

	//find mean color of each superpixel superpixel
	GetCurrentState()->superpixel_color = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC3);
	UpdateSuperpixelMeans();

	//Initialize the palette to 1 color = the mean of all input pixels
	cv::Vec3f first_color(0.0f,0.0f,0.0f);
	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			first_color+= GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x);
		}
	}
	
	//Initialize P(p_i), P(c_k), P(c_k|p_i)
	prob_o_ = 1.0f/(output_width_*output_height_);
	GetCurrentState()->prob_c.push_back(.5f);
	GetCurrentState()->prob_c.push_back(.5f);
	prob_co_.push_back(cv::vector<float>(output_width_*output_height_,.5f));
	prob_co_.push_back(cv::vector<float>(output_width_*output_height_,.5f));

	first_color *= prob_o_;
	GetCurrentState()->palette.push_back(first_color);
	GetCurrentState()->palette.push_back(first_color +
                                       GetMaxEigen(0).first*kSubclusterPertubation);
	GetCurrentState()->sub_superpixel_pairs.push_back(std::pair<int,int>(0,1));
	
	//set starting temperature
	temperature_ = kT0SafteyFactor*sqrt(2*GetMaxEigen(0).second);

	//set all pixels and colors to be unconstrained
	GetCurrentState()->locked_colors = 
    std::vector<bool>(max_palette_size_, false);
	GetCurrentState()->pixel_constraints = 
    std::vector<std::list<int> >(output_width_*output_height_);

	

}
void Pix::SaveToFile(std::string filename) {
	std::vector<std::string> extensions;
	
	cv::FileStorage file_storage(filename, cv::FileStorage::WRITE);

	//save original image
	//===================
	file_storage << "input_width_" << input_width_;
	file_storage << "input_height_" << input_height_;
	file_storage << "input_img_" << input_img_;

	//save output size
	file_storage << "output_width_" << output_width_;
	file_storage << "output_height_" << output_height_;

	//save palette
	file_storage <<"max_palette_size_" << max_palette_size_;
	file_storage << "palette" << "[";
		
	for(unsigned int i = 0; i< GetCurrentState()->palette.size(); ++i) {
		cv::Vec3f p = GetCurrentState()->palette[i];
		file_storage << p[0] <<  p[1]  << p[2];
	}
	file_storage <<"]";

	//save palette_prob
	{
		file_storage << "prob_c" << "[";
		for(unsigned int i = 0; i< GetCurrentState()->prob_c.size(); ++i) {
			file_storage << GetCurrentState()->prob_c[i]; 
		}
		file_storage <<"]";
	}

	//save locked colors
	file_storage << "locked_colors" << "[";
	for(unsigned int i = 0; i< GetCurrentState()->locked_colors.size(); ++i) {
		if(GetCurrentState()->locked_colors[i])
			file_storage << (int)i;
	}
	file_storage <<"]";
	
	//save superpixel position
	file_storage << "superpixel_pos" << GetCurrentState()->superpixel_pos;
	
	// save superpixel assignment
	file_storage << "palette_assign" << GetCurrentState()->palette_assign;
	
	//save pixel constraints
	file_storage << "pixel_constraints" << "[";
	for(unsigned int i = 0; i< GetCurrentState()->pixel_constraints.size(); ++i) {
		for(std::list<int>::iterator next = GetCurrentState()->pixel_constraints[i].begin(); next != GetCurrentState()->pixel_constraints[i].end(); ++next) {
			file_storage << *next;
		}
		file_storage << -1;
	}
	file_storage << "]";

	//save weights
	file_storage << "input_weights_" << input_weights_;

	//save iteration
	file_storage << "iteration" << GetCurrentState()->iteration;

	file_storage << "slic_factor_" << slic_factor_;
	file_storage << "sigma_color_" << sigma_color_;
	file_storage << "sigma_position_" << sigma_position_;
	file_storage << "smooth_pos_factor_" << smooth_pos_factor_;
  file_storage << "Saturation" << GetCurrentState()->saturation;

	file_storage.release();
}
void Pix::Iterate() {
	if(converged_flag_) return;

	//update segmentation
	UpdateSuperpixelMapping();
	UpdateSuperpixelMeans();
	
	//update palette
	AssociatePalette();
	float err_pal = RefinePalette();
	if(err_pal < kPaletteErrorTolerance) {
		if(temperature_ <= kTF)
			converged_flag_ = true;
		else
			temperature_ = std::max(temperature_*kDT,kTF);

		ExpandPalette();
	}


	GetCurrentState()->iteration++;
}
void Pix::AssociatePalette() {
	int current_palette_size = GetCurrentState()->palette.size();
	//used to store updated prob(index)
	std::vector<float> new_prob_c(current_palette_size, 0.0);
	//we will recalculate prob(index|p_s)
	prob_co_ = std::vector<std::vector<float>>(current_palette_size, 
             std::vector<float>(output_width_*output_height_, 0.0));
	double overT = -1.0f/temperature_;

	//associate SPs with colors in the palette
	//assign to each SP the color with the highest probability
	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			//for each SP: 
			int best_index = -1;
			double best_error;
			std::vector<double> probs;
			cv::Vec3f pixel = GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x);
			double sum_prob = 0;
			
			//get current SP pixel constraints
			std::list<int> constraints = 
        GetCurrentState()->pixel_constraints[vec2idx(cv::Vec2i(x,y))];
			//if there are no constraints, list all colors as possible constraints
			if(constraints.size() == 0) {
				for(int i = 0; i< current_palette_size; ++i) {
					constraints.push_back(i);
				}
			}

			for(std::list<int>::iterator nCol = constraints.begin(); nCol != constraints.end(); ++nCol) {				
				int index = *nCol;
				double color_error = norm(GetCurrentState()->palette[index], pixel);
				double prob = GetCurrentState()->prob_c[index]*exp(color_error*overT);
				probs.push_back(prob);
				sum_prob += prob;
				if(best_index == -1 || color_error < best_error) {
					best_index = index;
					best_error = color_error;
				}
			}
			//assign current SP the color with the highest probability
			GetCurrentState()->palette_assign.at<int>(y,x) = best_index;
			
			double prob_sp = superpixel_weights_.at<float>(y,x);
			int constraints_index = 0;
			for(std::list<int>::iterator nCol = constraints.begin(); nCol != constraints.end(); ++nCol) {
				int color_index = *nCol;
				double normalized_prob = probs.at(constraints_index)/sum_prob;
				prob_co_[color_index][vec2idx(cv::Vec2i(x,y))] = normalized_prob;
				new_prob_c[color_index] += prob_sp*normalized_prob;
				constraints_index++;
			}
		}
	}
	GetCurrentState()->prob_c = new_prob_c;
}
std::vector<cv::Vec3f> Pix::GetPalette() {
	std::vector<cv::Vec3f> effective_palette;
	effective_palette = GetCurrentState()->palette;
	if(!palette_maxed_flag_) {
		effective_palette.clear();
		for(int i = 0; i< GetCurrentState()->sub_superpixel_pairs.size(); ++i) {
			int index_1 = GetCurrentState()->sub_superpixel_pairs[i].first;
			int index_2 = GetCurrentState()->sub_superpixel_pairs[i].second;
			cv::Vec3f color_1 = GetCurrentState()->palette[index_1];
			cv::Vec3f color_2 = GetCurrentState()->palette[index_2];
			float weight_1 = GetCurrentState()->prob_c[index_1];
			float weight_2 = GetCurrentState()->prob_c[index_2];
			float total_weight = weight_1+weight_2;
			weight_1 /= total_weight;
			weight_2 /= total_weight;

			cv::Vec3f average_color(weight_1*color_1[0]+weight_2*color_2[0], 
                              weight_1*color_1[1]+weight_2*color_2[1], 
                              weight_1*color_1[2]+weight_2*color_2[2]);

			effective_palette.push_back(average_color);
		}
	}
	std::vector<cv::Vec3f> rgbPal;
	for(int i = 0; i< effective_palette.size(); ++i) {
		cv::Vec3f lab = effective_palette.at(i);
		lab[1] *= GetCurrentState()->saturation;
		lab[2] *= GetCurrentState()->saturation;
		cv::Vec3f bgr = lab2rgb(lab);
		rgbPal.push_back(cv::Vec3f(bgr[2],bgr[1],bgr[0]));
	}
	return rgbPal;
}	
void Pix::GetOutputImage(cv::Mat& img) {
	cv::Mat lab(cv::Size(output_width_,output_height_), CV_32FC3);
	std::vector<cv::Vec3f> averaged_palette = GetAveragedPalette();
	for(int i = 0; i<averaged_palette.size();++i) {
		averaged_palette[i][1] *= GetCurrentState()->saturation;
		averaged_palette[i][2] *= GetCurrentState()->saturation;
	}

	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			lab.at<cv::Vec3f>(y,x) = 
        averaged_palette[GetCurrentState()->palette_assign.at<int>(y,x)];
		}
	}
	cv::Mat rgb; 
	cv::cvtColor(lab, rgb, CV_Lab2RGB);
	rgb.convertTo(img, CV_8UC3, 255.0);
}
void Pix::GetRegionImage(cv::Mat& img) {
  cv::Mat temp;
  cvtColor(input_img_, temp, CV_Lab2RGB);
	temp.convertTo(img, CV_8UC3, 255.0);
  for(int y = 0; y< input_height_; ++y) {
		for(int x = 0; x<input_width_; ++x) {
      cv::Vec2i cluster = region_map_.at<cv::Vec2i>(y,x);
			if(x+1 < region_map_.cols) {
				cv::Vec2i c2 = region_map_.at<cv::Vec2i>(y,x+1);
				if (c2[0] != cluster[0] || c2[1] != cluster[1]) {
					img.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
				}
			}
			if(y+1 < region_map_.rows) {
				cv::Vec2i c2 = region_map_.at<cv::Vec2i>(y+1,x);
				if (c2[0] != cluster[0] || c2[1] != cluster[1]) {
					img.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
				}
			}
		}
	}
}

void Pix::UpdateSuperpixelMapping() {
	region_map_ = 
    cv::Mat(cv::Size(input_width_, input_height_),CV_32SC2,cv::Scalar(-1.0));
	cv::vector<cv::Vec3f> averaged_palette = GetAveragedPalette();

	//for each superpixel, update all pixels in a 2sx2s region
	cv::Mat distance = 
    cv::Mat(cv::Size(input_width_, input_height_), CV_32FC1, cv::Scalar(-5.0f));
	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			cv::Vec2f pos = GetCurrentState()->superpixel_pos.at<cv::Vec2f>(y,x);
			int min_x = std::max(0.0f,pos[0]-range_);
			int min_y = std::max(0.0f,pos[1]-range_);
			int max_x = std::min<int>(input_width_-1,(int)(pos[0]+range_));
			int max_y = std::min<int>(input_height_-1,(int)(pos[1]+range_));
			cv::Vec3f superpixel_color = 
        averaged_palette[GetCurrentState()->palette_assign.at<int>(y,x)];
			
			int idx = vec2idx(cv::Vec2i(x,y));

			for(int yy = min_y; yy<= max_y; ++yy) {
				for(int xx = min_x; xx<=max_x; ++xx) {
					cv::Vec3f pixel_color = input_img_.at<cv::Vec3f>(yy,xx);
					float color_error = norm(pixel_color, superpixel_color);
					float dist_err = (float) cv::norm(cv::Vec2f((float)xx,(float)yy)-pos);
					float error = color_error + slic_factor_/range_*dist_err;

					if(distance.at<float>(yy,xx) < 0 || error < distance.at<float>(yy,xx)) {
						distance.at<float>(yy,xx) = error;
						region_map_.at<cv::Vec2i>(yy,xx) = cv::Vec2i(x,y);
					}
				}
			}
		}
	}
	//store input pixels in superpixel regions
	region_list_ = 
    std::vector<std::vector<cv::Vec2i> >(output_width_*output_height_);
	for(int y = 0; y< input_height_; ++y) {
		for(int x = 0; x<input_width_; ++x) {
			cv::Vec2i superpixel = region_map_.at<cv::Vec2i>(y,x);
			int index = vec2idx(superpixel);

			if(index == -1) {
				int i = (int) ( x/(float)input_width_*output_width_);
				int j = (int) ( y/(float)input_height_*output_height_ );
				region_map_.at<cv::Vec2i>(y,x) = cv::Vec2i(i,j);
				index = vec2idx(cv::Vec2i(i,j));
			}
			region_list_[index].push_back(cv::Vec2i(x,y));
		}
	}
}
void Pix::UpdateSuperpixelMeans() {
	cv::Mat color_sums = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC3,cv::Scalar(0.0f));
	cv::Mat pos_sums = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC2,cv::Scalar(0.0f));
  cv::Mat weights = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC1,cv::Scalar(0.0f));


	superpixel_weights_ = 
    cv::Mat(cv::Size(output_width_, output_height_),CV_32FC1, cv::Scalar(0.0f));
	//total them up
	for(int y = 0; y < input_height_; ++y) {
		for(int x = 0; x < input_width_; ++x) {
			cv::Vec2i superpixel = region_map_.at<cv::Vec2i>(y,x);
			cv::Vec3f pixel_color = input_img_.at<cv::Vec3f>(y,x);
			color_sums.at<cv::Vec3f>(superpixel[1],superpixel[0]) += pixel_color;
			pos_sums.at<cv::Vec2f>(superpixel[1],superpixel[0]) += 
        cv::Vec2f((float) x,(float) y);
			weights.at<float>(superpixel[1],superpixel[0]) += 1.0f;
			superpixel_weights_.at<float>(superpixel[1],superpixel[0]) += 
        input_weights_.at<float>(y,x);
			
		}
	}
	//find the average
	int total_weight = 0;
	for(int y = 0; y<color_sums.rows; ++y) {
		for(int x = 0; x<color_sums.cols; ++x) {
			float w = weights.at<float>(y,x);
      if(w == 0) {
        int input_x = x/(float)output_width_*input_width_;
        int input_y = x/(float)output_height_*input_height_;
        cv::Vec3f input_col = input_img_.at<cv::Vec3f>(input_y,input_x);
        GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x) = input_col;
      } else {
        float wn = 1.0/w;
			  GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x) = 
          color_sums.at<cv::Vec3f>(y,x) * wn;
			  GetCurrentState()->superpixel_pos.at<cv::Vec2f>(y,x) = 
          pos_sums.at<cv::Vec2f>(y,x) * wn;	
			  superpixel_weights_.at<float>(y,x) *= wn;
			  total_weight += superpixel_weights_.at<float>(y,x);
      }
		}
	}
	for(int y = 0; y<color_sums.rows; ++y) {
		for(int x = 0; x<color_sums.cols; ++x) {
			superpixel_weights_.at<float>(y,x) /= total_weight;
		}
	}

	SmoothSuperpixelPositions();
	SmoothSuperpixelColors();	
}
void Pix::SmoothSuperpixelPositions() {
	cv::Mat new_superpixel_pos = cv::Mat(GetCurrentState()->superpixel_pos.size(), 
                                      GetCurrentState()->superpixel_pos.type());
	for(int i = 0; i<output_width_; ++i) {
		for(int j = 0; j<output_height_; ++j) {
			cv::Vec2f sum(0,0);
			float count = 0.0f;

			//average neighboring vertices (avoid going out of image bounds)
			if(i > 0) {
				sum += GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j,i-1);
				count += 1.0f;
			}
			if(i< GetCurrentState()->superpixel_pos.cols -1) {
				sum += GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j,i+1);
				count += 1.0f;
			}
			if(j > 0) {
				sum += GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j-1,i);
				count += 1.0f;
			}
			if(j<GetCurrentState()->superpixel_pos.rows - 1) {
				sum += GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j+1,i);
				count += 1.0f;
			}
			sum[0] /= count;
			sum[1] /= count;
			
			//Move the current superpixels position a percentage of the
			//way to the centroid of it's neighbors. If it is missing a
			//neighbor in the x or y direction, do not smooth in that
			//direction
			cv::Vec2f orig = GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j,i);
			cv::Vec2f nPos(0,0);
			if(i == 0 || i == GetCurrentState()->superpixel_pos.cols -1) {
				nPos[0] = GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j,i)[0];
			} else {
				nPos[0] = (1.0f-smooth_pos_factor_)*orig[0] + smooth_pos_factor_*sum[0];
			}
			if(j == 0 || j == GetCurrentState()->superpixel_pos.rows - 1) {
				nPos[1] = GetCurrentState()->superpixel_pos.at<cv::Vec2f>(j,i)[1];
			} else {
				nPos[1] = (1.0f-smooth_pos_factor_)*orig[1] + smooth_pos_factor_*sum[1];
			}
			new_superpixel_pos.at<cv::Vec2f>(j,i) = nPos;
		}
	}
	//update the SP position matrix with the smoothed locations
	GetCurrentState()->superpixel_pos = new_superpixel_pos;
}
void Pix::SmoothSuperpixelColors() {
	cv::Mat new_superpixel_colors(GetCurrentState()->superpixel_color.size(), 
                                GetCurrentState()->superpixel_color.type());
	for(int i = 0; i<GetCurrentState()->superpixel_color.cols; ++i)
	{
		for(int j = 0; j<GetCurrentState()->superpixel_color.rows; ++j)
		{
			 
			//get bounds of 3x3 kernel (make sure we don't go off the image)
			int min_x = std::max(0,i-1);
			int max_x = std::min(output_width_-1,i+1);
			int min_y = std::max(0,j-1);
			int max_y = std::min(output_height_-1,j+1);

			//Initialize
			cv::Vec3f sum(0,0,0);
			float weight = 0;

			//get current SP color and (grid) position
			cv::Vec3f superpixel_color = 
        GetCurrentState()->superpixel_color.at<cv::Vec3f>(j,i);
			cv::Vec2f p((float) j, (float) i);

			//get bilaterally weighted average color of SP neighborhood
			for(int ii = min_x; ii<= max_x; ++ii)
			{
				for(int jj = min_y; jj<=max_y; ++jj)
				{
					
				cv::Vec3f c_n = GetCurrentState()->superpixel_color.at<cv::Vec3f>(jj,ii);
				float d_color = norm(superpixel_color,c_n);
				float w_color = gaussian(d_color,sigma_color_,0.0f);
				float d_pos = (float)norm(cv::Vec2i(i,j) - cv::Vec2i(ii,jj));
				float w_pos = gaussian(d_pos, sigma_position_, 0.0f);
				float w_total = w_color*w_pos;

				weight += w_total;
				sum += c_n*w_total;
				}
			}
			sum *= 1.0/weight;
			new_superpixel_colors.at<cv::Vec3f>(j,i) = sum;
		}
	}
	//update the SP mean colors with the smoothed values
	GetCurrentState()->superpixel_color = new_superpixel_colors;
}
float Pix::RefinePalette() {
	int current_palette_size = GetCurrentState()->palette.size();
	//used to store weighted averages of SP for refinement step
	std::vector<cv::Vec3d> color_sums(current_palette_size, cv::Vec3d(0.0,0.0,0.0)); 
	
	//take a weighted average of all superpixels, based on their probability of
	//association
	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			float prob_sp = superpixel_weights_.at<float>(y,x);
			cv::Vec3d pixel_color = 
        GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x);
			for(int c = 0; c<GetCurrentState()->palette.size(); ++c) {
				double w = prob_sp*prob_co_[c][vec2idx(cv::Vec2i(x,y))];
				color_sums.at(c) += pixel_color*w;
			}
		}
	}

	//update the palette colors
	float palette_error = 0;
	for(int i = 0; i< color_sums.size();++i) {
		//if the color is not locked and prob(c) > 0, update it
		cv::Vec3d color = GetCurrentState()->palette[i];
		if(!(GetCurrentState()->locked_colors[i]) && 
       GetCurrentState()->prob_c[i] > 0) {
			cv::Vec3d new_color = color_sums[i] * (1.0/GetCurrentState()->prob_c[i]);
			GetCurrentState()->palette[i] = new_color;
			palette_error += norm(color - new_color);
		}
	}
	return palette_error;
}
void Pix::ExpandPalette()
{
	if(palette_maxed_flag_) return;
	
	//record which pair needs to be split
	//and the distance between the two subsuperpixels
	std::vector<std::pair<float,int> > splits;
	int num_colors = GetCurrentState()->palette.size();
	int num_subclusters = GetCurrentState()->sub_superpixel_pairs.size();
	for(int index = 0; index< num_subclusters;++index) {
		int index_1 = GetCurrentState()->sub_superpixel_pairs[index].first;
		int index_2 = GetCurrentState()->sub_superpixel_pairs[index].second;
		cv::Vec3f color_1 = GetCurrentState()->palette[index_1];
		cv::Vec3f color_2 = GetCurrentState()->palette[index_2];
			
		float subcluster_error = norm(color_1-color_2);
		//mark pair as splitting if distance between exceeds a threshold
		if(subcluster_error > kSubclusterTolerance) {
			splits.push_back(std::pair<float,int>(subcluster_error,index));
		} else { //otherwise make the 2nd SC a slight permutation of the first
      GetCurrentState()->palette[index_2] += 
        GetMaxEigen(index_1).first*kSubclusterPertubation;
		}
	}
	//split qualifiying subsuperpixels
	//for the case when there are more superpixels that need to be
	//split than room in the palette, we split subsuperpixels with
	//the greatest distance first
	sort(splits.begin(), splits.end());
	for(int i = splits.size()-1; i>=0; i--) {
		SplitColor(splits[i].second);
		
		//condense palette if maximum size has been reached
		if(GetCurrentState()->palette.size() >= 2*max_palette_size_) {
			CondensePalette();
			break; //do not continue to Process superpixel splits
		}
	}
}
void Pix::SplitColor(int pair_index) {
	//remove subsuperpixels if max palette size is reached
	//and represent each color as a single superpixel
	
	int index_1 = GetCurrentState()->sub_superpixel_pairs[pair_index].first;
	int index_2 = GetCurrentState()->sub_superpixel_pairs[pair_index].second;

	//get next two spots in the palette for the
	//two additional subsuperpixels generated by
	//splitting
	int next_index1 = GetCurrentState()->palette.size();
	int next_index2 = next_index1+1;
	cv::Vec3f color_1 = GetCurrentState()->palette[index_1];
	cv::Vec3f color_2 = GetCurrentState()->palette[index_2];

	//create a subsuperpixel for each of the two new
	//colors, set to slight permutations of the
	//old subsuperpixels' colors
	cv::Vec3f subcluster_color_1 = 
    color_1 + GetMaxEigen(index_1).first*kSubclusterPertubation;
	cv::Vec3f subcluster_color_2 = 
    color_2 + GetMaxEigen(index_2).first*kSubclusterPertubation;

	//reconstruct first pair
	GetCurrentState()->palette.push_back(subcluster_color_1);
	GetCurrentState()->sub_superpixel_pairs[pair_index].second = next_index1;
	GetCurrentState()->prob_c[index_1]*=.5f; 
	GetCurrentState()->prob_c.push_back(GetCurrentState()->prob_c[index_1]);
	prob_co_.push_back(prob_co_[index_1]);

	//reconstruct second pair
	GetCurrentState()->palette.push_back(subcluster_color_2);
  std::pair<int,int> new_pair(index_2, next_index2);
	GetCurrentState()->sub_superpixel_pairs.push_back(new_pair);
	GetCurrentState()->prob_c[index_2]*=.5f;
	GetCurrentState()->prob_c.push_back(GetCurrentState()->prob_c[index_2]);
	prob_co_.push_back(prob_co_[index_2]);
}
void Pix::CondensePalette() {
	palette_maxed_flag_ = true;
	std::vector<cv::Vec3f> old_palette = GetCurrentState()->palette;
	std::vector<cv::Vec3f> new_palette;
	std::vector<std::vector<float> > new_prob_co;
	std::vector<float> new_prob_c;
	cv::Mat nPaletteAssign(GetCurrentState()->palette_assign.size(),CV_32FC3);
	for(int j = 0; j < GetCurrentState()->sub_superpixel_pairs.size();++j) {
		//average the subsuperpixel colors into a single color
		//weighted by p(c) of each subsuperpixel
		int index_1 = GetCurrentState()->sub_superpixel_pairs[j].first;
		int index_2 = GetCurrentState()->sub_superpixel_pairs[j].second;
		float weight_1 = GetCurrentState()->prob_c[index_1];
		float weight_2 = GetCurrentState()->prob_c[index_2];
		float total_weight = weight_1+weight_2;
		weight_1 /= total_weight;
		weight_2 /= total_weight;
		new_palette.push_back((old_palette[index_1] *weight_1) +
                          (old_palette[index_2] * weight_2));


		//update the probability of the single superpixel
		new_prob_c.push_back(GetCurrentState()->prob_c[index_1] + 
                         GetCurrentState()->prob_c[index_2]);
		new_prob_co.push_back(prob_co_[index_1]);

		//for each SP, if it was assigned to either subsuperpixel, assign it to the merged superpixel
		for(int y = 0; y<output_height_; ++y) {
			for(int x = 0; x<output_width_; ++x) {
				if (GetCurrentState()->palette_assign.at<int>(y,x) == index_1 
            || GetCurrentState()->palette_assign.at<int>(y,x) == index_2) {
					nPaletteAssign.at<int>(y,x) = j;
				}
			}
		}
	}
	GetCurrentState()->palette = new_palette;
	GetCurrentState()->palette_assign = nPaletteAssign;
	GetCurrentState()->prob_c = new_prob_c;
	prob_oc_ = new_prob_co;
}
std::pair<cv::Vec3f,float> Pix::GetMaxEigen(int palette_index) {
	//for every output pixel
	cv::Mat matrix(cv::Size(3,3),CV_64FC1,cv::Scalar(0.0f));
	float sum = 0;
	for(int y = 0; y<output_height_; ++y) {
		for(int x = 0; x<output_width_; ++x) {
			//get prob(output pixel|palette color)
			float prob_oc = prob_co_[palette_index][vec2idx(cv::Vec2i(x,y))] 
                      * prob_o_ / GetCurrentState()->prob_c[palette_index];
			sum += prob_oc;
			//construct 3x3 matrix and add to sum
			cv::Vec3d color_error = GetCurrentState()->palette[palette_index] 
              - GetCurrentState()->superpixel_color.at<cv::Vec3f>(y,x);
			color_error[0] = abs(color_error[0]);
			color_error[1] = abs(color_error[1]);
			color_error[2] = abs(color_error[2]);
			matrix.at<double>(0,0) += prob_oc*color_error[0]*color_error[0];
			matrix.at<double>(1,0) += prob_oc*color_error[1]*color_error[0];
			matrix.at<double>(2,0) += prob_oc*color_error[2]*color_error[0];
			matrix.at<double>(0,1) += prob_oc*color_error[0]*color_error[1];
			matrix.at<double>(1,1) += prob_oc*color_error[1]*color_error[1];
			matrix.at<double>(2,1) += prob_oc*color_error[2]*color_error[1];
			matrix.at<double>(0,2) += prob_oc*color_error[0]*color_error[2];
			matrix.at<double>(1,2) += prob_oc*color_error[1]*color_error[2];
			matrix.at<double>(2,2) += prob_oc*color_error[2]*color_error[2];
		}
	}

	//get critical temperature = largest eigenvalue of convariance matrix
	cv::Mat values;
	cv::Mat vectors;
	cv::eigen(matrix,values, vectors);

	cv::Vec3f eVec = cv::Vec3f(vectors.at<double>(0,0),
                             vectors.at<double>(0,1),
                             vectors.at<double>(0,2));
	float len = norm(eVec);
	if(len > 0)
		eVec *= (1.0/len);
	float eVal = abs(values.at<double>(0,0));
	
	return std::pair<cv::Vec3f, float>(eVec, eVal);
}

std::vector<cv::Vec3f> Pix::GetAveragedPalette() {
	std::vector<cv::Vec3f> averaged_palette;
	averaged_palette = GetCurrentState()->palette;
	if(!palette_maxed_flag_) {
		for(int i = 0; i< GetCurrentState()->sub_superpixel_pairs.size(); ++i) {
			int index_1 = GetCurrentState()->sub_superpixel_pairs[i].first;
			int index_2 = GetCurrentState()->sub_superpixel_pairs[i].second;
			cv::Vec3f color_1 = GetCurrentState()->palette[index_1];
			cv::Vec3f color_2 = GetCurrentState()->palette[index_2];
			float weight_1 = GetCurrentState()->prob_c[index_1];
			float weight_2 = GetCurrentState()->prob_c[index_2];
			float total_weight = weight_1+weight_2;
			weight_1 /= total_weight;
			weight_2 /= total_weight;

			cv::Vec3f average_color(weight_1*color_1[0]+weight_2*color_2[0], 
                              weight_1*color_1[1]+weight_2*color_2[1], 
                              weight_1*color_1[2]+weight_2*color_2[2]);

			averaged_palette[index_1] = average_color;
			averaged_palette[index_2] = average_color;
		}
	}
	return averaged_palette;
}

