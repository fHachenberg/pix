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

#include "PixUI.h"
#include "cinder\ImageIo.h"
#include "cinder\gl\gl.h"
#include "const.h"
#include "Resources.h"
#include "CinderOpenCV.h"

PixUI::~PixUI() {
  if(pix_)
    delete pix_;
}
void PixUI::prepareSettings(Settings * settings) {
  settings->setTitle("Pix");
  settings->setWindowSize(settings->getDisplay()->getWidth()*.75, 
    settings->getDisplay()->getHeight()*.75);
  settings->setWindowPos(settings->getDisplay()->getWidth()*.125,
    settings->getDisplay()->getHeight()*.125);
}
void PixUI::setup() {
  srand(time(NULL));

  max_palette_size_ = 8;
  output_width_ = 32;
  output_height_ = 32;;
  slic_factor_ = 45;
  state_ = NODATA;
  saturation_ = 1.1f;
  smooth_factor_ = .4f;
  sigma_color_ = 4.0f;
  sigma_position_ = .87f;
  fixed_are_visible_ = false;
  brush_size_ = 1;
  superpixels_are_visible_ = false;
  output_is_hidden_ = false;
  pix_ = NULL;
  output_img_is_outdated_ = false;
  weight_img_is_outdated_ = false;

  InitializeBar();
  UpdateBar();

  lock_img_ = gl::Texture(loadImage(loadResource(kMainViewLOCK)));
  lock_img_.setMagFilter(GL_NEAREST);
  select_img_ = gl::Texture(loadImage(loadResource(kMainViewSELC)));
  select_img_.setMagFilter(GL_NEAREST);


  font_ = ci::Font(kFontStyle, kFontSize);

  TextLayout palette_text;
  palette_text.setFont(font_);
  palette_text.setColor(ColorA(1.0,1.0,1.0,1.0));
  palette_text.addCenteredLine("Palette");
  Surface8u palette_rendered = palette_text.render(true,false);
  palette_label_ = gl::Texture(palette_rendered);
  palette_label_.setMagFilter(GL_NEAREST);
  palette_label_.setMinFilter(GL_NEAREST);

  TextLayout preview_text;
  preview_text.setFont(font_);
  preview_text.setColor(ColorA(1.0,1.0,1.0,1.0));
  preview_text.addCenteredLine("Preview");
  Surface8u preview_rendered = preview_text.render(true,false);
  preview_label_ = gl::Texture(preview_rendered);
  preview_label_.setMagFilter(GL_NEAREST);
  preview_label_.setMinFilter(GL_NEAREST);

  glEnable (GL_BLEND); 
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  transparency_ = 0.0f;
  brush_weight_ = 0.0f;
}
void PixUI::update() {
  //only send saturation update when value has changed
  if(state_ != NODATA && state_!= WEIGHTING && 
    previous_saturation_ != saturation_) {
      pix_->SetSaturation(saturation_);
      previous_saturation_ = saturation_;
      output_img_is_outdated_ = true;
  }

  //maintain aspect ratio of output
  if(state_ != NODATA && previous_width_ != output_width_) {
    previous_width_ = output_width_;
    output_height_ = output_width_/input_img_.getAspectRatio() + .5f;
    previous_height_ = output_height_;
  }
  else if(state_ != NODATA && previous_height_ != output_height_) {
    previous_height_ = output_height_;
    output_width_ = output_height_*input_img_.getAspectRatio() + .5f; 
    previous_width_ = output_width_;
  }

  //handle different states
  switch(state_) {
  case NODATA:
    ; 
    set_message( "Click New Project or Load Project to begin.");
  case WEIGHTING:
    PaintWeight();
    break;
  case ITERATING:
    pix_->Iterate();
    output_img_is_outdated_ = true;
    break;
  case EDIT:
    PaintPixels();
    break;
  }


  if(state_ == ITERATING && pix_->hasConverged()) {
    SetState(EDIT);
    brush_size_ = std::max(brush_size_,1.0f);
    Reassociate();
  }

  //update the output image as needed
  if(output_img_is_outdated_) {
    cv::Mat o;
    pix_->GetOutputImage(o);
    output_img_ = fromOcv(o);
    output_img_.setMagFilter(GL_NEAREST);

    cv::Mat r;
    pix_->GetSuperpixelImage(r);
    average_img_ = fromOcv(r);
    average_img_.setMagFilter(GL_NEAREST);

    output_img_is_outdated_ = false;
  }

  //update the weight image as needed
  if(weight_img_is_outdated_) {
    UpdateWeightImg();
    weight_img_is_outdated_ = false;
  }

}
void PixUI::draw(){
  gl::clear(Color(0.3f,0.3f,0.3f));
  glDisable(GL_DEPTH_TEST);
  DrawBackground();

  switch(state_)
  {
  case NODATA:
    break;
  case WEIGHTING:
    DrawWeightingMode();
    break;
  case ITERATING:
    DrawIteratingMode();
    break;
  case EDIT:
    DrawEditMode();
    break;
  }


  bar_.setPosition(kBarX*getWindowWidth(), kBarY*getWindowHeight());
  bar_.setSize(kBarW*getWindowWidth(), kBarH*getWindowHeight());
  bar_.draw();

  DrawMessage();
}
void PixUI::keyUp(KeyEvent event) {
  if(state_ != EDIT) return;

  switch(event.getCode()) {
  case KeyEvent::KEY_LALT:
    superpixels_are_visible_ = false;
    break;
  case KeyEvent::KEY_SPACE:
    output_is_hidden_ = false;
    break;
  case KeyEvent::KEY_y:
    pix_->Redo();
    Reassociate();
    break;
  case KeyEvent::KEY_z:
    pix_->Undo();
    Reassociate();
    break;
  }
}
void PixUI::keyDown(KeyEvent event) {
  switch(event.getCode()) {
  case KeyEvent::KEY_LALT:
    superpixels_are_visible_ = true;
    break;
  case KeyEvent::KEY_SPACE:
    output_is_hidden_ = true;
    break;
  }
}
void PixUI::mouseDown(MouseEvent e) {
  if(state_ == EDIT) {
    int color_index = GetPaletteClick(e.getPos());
    cv::Vec2i pixel = GetImgClick(e.getPos(), output_width_, output_height_);
    if(color_index >= 0) {
      if(e.isLeft()) {

        if(!e.isShiftDown())
          selected_colors = std::vector<bool>(selected_colors.size(), false);
        selected_colors[color_index] = !selected_colors[color_index];
        selected_list.clear();
        for(int i = 0; i<selected_colors.size();++i) {
          if(selected_colors[i])
            selected_list.push_back(i);
        }
        if(selected_list.size() == 1) {
          bar_.setOptions("Edit Color", "visible = true");
          cv::Vec3f pixel = pix_->GetPalette()[selected_list.front()];
          color_picker_ = ci::Color(pixel[0],pixel[1],pixel[2]);
        } else {
          bar_.setOptions("Edit Color", "visible = false");
        }
      } else if(e.isRight()) {
        pix_->SaveState();
        pix_->SetColorLock(color_index, !pix_->GetColorLock(color_index));
      }
    } else if(pixel[0] >= 0 && pixel[0] < pix_->get_output_width() 
      && pixel[1] >= 0 && pixel[1] < pix_->get_output_height()) {
        if(e.isAltDown()) {
          if(selected_list.size() == 1) {
            pix_->SaveState();
            pix_->SetColorFromSP(selected_list.front(), pixel);
            pix_->SetColorLock(selected_list.front(), true);
            Reassociate();
          }
        } else {
          if(e.isLeft() || e.isRight()) {
            MOUSECLICK t;
            pix_->SaveState();
            if(e.isLeft()) t = LEFT;
            else t = RIGHT;
            paint_queue_.push(std::pair<cv::Vec2i, MOUSECLICK>(pixel,t));
          }	
        }

    }

  }
  if(state_ == WEIGHTING) {
    cv::Vec2i pixel = GetImgClick(e.getPos(), 
      input_img_.getWidth(), 
      input_img_.getHeight());
    if(pixel[0] >= 0 && pixel[0] < input_img_.getWidth() 
      && pixel[1] >= 0 && pixel[1] < input_img_.getHeight()) {

        paint_queue_.push(std::pair<cv::Vec2i, MOUSECLICK>(pixel, LEFT));
    }
  }
}
void PixUI::mouseWheel(MouseEvent e) {
  if(state_ == EDIT) {
    brush_size_ += .5f*e.getWheelIncrement();
    brush_size_ = std::max(brush_size_, 1.0f);
  }
  if(state_ == WEIGHTING) {
    if(e.isShiftDown()) {
      brush_weight_ += .05f*e.getWheelIncrement();
      brush_weight_ = std::min(1.0f,std::max(0.0f,brush_weight_));
    } else {
      brush_size_ += .01f*input_img_.getWidth()*e.getWheelIncrement();
      brush_size_ = std::max(brush_size_,0.5f);
    }
  }
}
void PixUI::mouseDrag(MouseEvent e) {
  if(state_ != EDIT && state_ != WEIGHTING) return;

  MOUSECLICK t; 
  if(e.isLeftDown()) t = LEFT;
  else if(e.isRightDown()) t = RIGHT;
  else return;

  int w, h;
  switch(state_) {
  case WEIGHTING:
    w = input_weights.cols;
    h = input_weights.rows;
    break;
  case EDIT:
    w = output_width_;
    h = output_height_;
  }

  cv::Vec2i click = GetImgClick(e.getPos(),w,h);

  if(paint_queue_ .empty() || paint_queue_.back().first != click || paint_queue_.back().second != t)
    paint_queue_.push((std::pair<cv::Vec2i, MOUSECLICK>(click, t)));

}
void PixUI::Process(){
  if(pix_ == NULL) {
    //toOcv won't convert to CV32FC3
    cv::Mat in8U = toOcv(input_img_);
    cv::Mat in32;
    in8U.convertTo(in32, CV_32FC3, 1.0/255.0);

    pix_ = new Pix(in32, output_width_, output_height_, max_palette_size_);
    pix_->SetBilateralParams(sigma_color_, sigma_position_);
    pix_->set_laplacian_factor(smooth_factor_);
    pix_->setSlicFact(slic_factor_);
    pix_->set_input_weights(input_weights);
    pix_->SetSaturation(saturation_);
    pix_->Initialize();

    paint_queue_ = std::queue<std::pair<cv::Vec2i, MOUSECLICK>>();
    selected_colors = std::vector<bool>(max_palette_size_, false);
    selected_list.clear();
    brush_size_ = .5;

  } else {
    pix_->SaveState();
  }

  SetState(ITERATING);
}
void PixUI::Reassociate() {
  output_img_is_outdated_ = true;
  pix_->AssociatePalette();
}
void PixUI::InitializeBar() {
  bar_ = interface("Menu", ci::Vec2i(220,400));
  bar_.setOptions("", "movable = false resizable = false fontresizable = false iconifiable = false");
  bar_.addButton("New Project",std::bind(&PixUI::NewProj, this),"group=File help = 'Create a New Project from a source image'");
  bar_.addButton("Load Project", std::bind(&PixUI::LoadProj, this), "group = File help = 'Load an existing Project'");
  bar_.addButton("Save Project",std::bind(&PixUI::SaveProj,this), "group=File help = 'save the project'");
  bar_.addParam("Output Width", &output_width_, "min = 2, max = 1000 group=Parameters help = 'The width of the output image'");
  bar_.addParam("Output Height", &output_height_, "min = 2, max = 1000 group=Parameters help = 'The height of the output image'");
  bar_.addParam("Palette Size",&max_palette_size_, "min= 1, max = 64 group= Parameters help = 'The size of the output palette'");
  bar_.addButton("Reset",std::bind(&PixUI::Reset, this), "group = File help = 'Resets the output and palette size, losing progress'");
  bar_.addButton("Save Image", std::bind(&PixUI::SaveImg,this), "group=File help = 'Save the results as a PNG image'");
  bar_.addButton("Process", std::bind(&PixUI::Process, this), "group = Edit help = 'Process using our algorithm'");
  bar_.addParam("Brush Size", &brush_size_, "min = .5 max = 500 step = .5 group = Edit help = 'The size of the PaintPixels brush'");
  bar_.addParam("Transparency", &transparency_, "min =0.0f max = 1.0f step = .01 group= Edit help= 'Use to overlay the original and output images'");
  bar_.addParam("Weight", &brush_weight_, "min = 0.0f max = 1.0f step = .01 group = Edit help = 'The strength of the Weighting Brush. Pixels painted with 1 will be the most imprortant, those painted with 0 will be the least");
  bar_.addParam("Show Fixed Pixels", &fixed_are_visible_, "group = Edit help = 'toggle visual cue of which pixels have been locked by painting. Locked pixels are not processed'");
  bar_.addParam("Color",&color_picker_, "group = 'Edit Color' help = 'Choose a color'");
  bar_.addButton("Apply Color", std::bind(&PixUI::SetColor, this), "group = 'Edit Color' help ='apply the chosen color to the currently selected color in the palette'");
  bar_.addParam("Saturation", &saturation_, "min = 1.0, max = 2.0, step = 0.01 group = Advanced help = 'How much the output is saturated. ex: 1.1 = 10% saturation'");
  bar_.addParam("SLIC Factor", &slic_factor_, "min = 1, max = 1000 group = Advanced help = 'When Processing, the importance of pixel location vs. pixel color. The higher the value, the more important pixel location becomes'");
  bar_.addParam("Smooth Factor", &smooth_factor_, "min = 0.0f, max = 1.0f, step = .01 group = Advanced help = 'During processing, the higher the smooth factor, the more regions try to retain their original neighborhoods'");
  bar_.addParam("Color Std", &sigma_color_, "min = 0.0f, max = 100f, step = .01 group = Advanced help = 'Bilateral filter color STD'");
  bar_.addParam("Position Std", &sigma_position_, "min = 0.0f, max = 100f, step = .01 group = Advanced help = 'Bilateral filter position STD'");
  bar_.setOptions("Advanced", "opened = false");
  bar_.setOptions("Edit Color", "visible = false");
}
void PixUI::UpdateBar() {
  switch (state_) {
  case NODATA:
    bar_.setOptions("Save Project", "visible = false");
    bar_.setOptions("Output Width", "visible = false");
    bar_.setOptions("Output Height", "visible = false");
    bar_.setOptions("Palette Size", "visible = false");
    bar_.setOptions("Reset", "visible = false");
    bar_.setOptions("Save Image", "visible = false");
    bar_.setOptions("Advanced", "visible = false");
    bar_.setOptions("Edit", "visible = false");
    bar_.setOptions("Parameters", "visible = false");
    break;
  case WEIGHTING:
    bar_.setOptions("Output Width", "readonly = false");
    bar_.setOptions("Output Height", "readonly = false");
    bar_.setOptions("Palette Size", "readonly = false");
    bar_.setOptions("Output Width", "visible = true");
    bar_.setOptions("Output Height", "visible = true");
    bar_.setOptions("Palette Size", "visible = true");
    bar_.setOptions("Save Project", "visible = false");
    bar_.setOptions("Save Image", "visible = false");
    bar_.setOptions("Reset", "visible = true");
    bar_.setOptions("Advanced", "visible = true");
    bar_.setOptions("Edit", "visible = true");
    bar_.setOptions("Transparency", "visible = false");
    bar_.setOptions("Weight", "visible = true");
    bar_.setOptions("Show Fixed Pixels", "visible = false");
    bar_.setOptions("Parameters", "visible = true");
    bar_.setOptions("SLIC Factor", "readonly = false");
    bar_.setOptions("Smooth Factor", "readonly = false");
    bar_.setOptions("Color Std", "readonly = false");
    bar_.setOptions("Position Std", "readonly = false");

    break;
  case ITERATING:
    bar_.setOptions("Output Width", "readonly = true");
    bar_.setOptions("Output Height", "readonly = true");
    bar_.setOptions("Palette Size", "readonly = true");
    bar_.setOptions("Output Width", "visible = true");
    bar_.setOptions("Output Height", "visible = true");
    bar_.setOptions("Palette Size", "visible = true");
    bar_.setOptions("Save Project", "visible = false");
    bar_.setOptions("Reset", "visible = true");
    bar_.setOptions("Save Image", "visible = false");
    bar_.setOptions("Advanced", "visible = false");
    bar_.setOptions("Edit", "visible = false");
    bar_.setOptions("Parameters", "visible = true");
    break;
  case EDIT:
    bar_.setOptions("Output Width", "visible = true");
    bar_.setOptions("Output Height", "visible = true");
    bar_.setOptions("Palette Size", "visible = true");
    bar_.setOptions("Output Width", "readonly = true");
    bar_.setOptions("Output Height", "readonly = true");
    bar_.setOptions("Palette Size", "readonly = true");
    bar_.setOptions("Save Project", "visible = true");
    bar_.setOptions("Save Image", "visible = true");
    bar_.setOptions("Reset", "visible = true");
    bar_.setOptions("Edit", "visible = true");
    bar_.setOptions("Transparency", "visible = true");
    bar_.setOptions("Weight", "visible = false");
    bar_.setOptions("Show Fixed Pixels", "visible = true");
    bar_.setOptions("Advanced", "visible = true");
    bar_.setOptions("Parameters", "visible = true");
    bar_.setOptions("SLIC Factor", "readonly = true");
    bar_.setOptions("Smooth Factor", "readonly = true");
    bar_.setOptions("Color Std", "readonly = true");
    bar_.setOptions("Position Std", "readonly = true");
  }
}
void PixUI::DrawBackground(){


  int width = getWindowWidth();
  int height = getWindowHeight();

  gl::color(kBackgroundColor);
  gl::drawSolidRect(Rectf(kMainViewX*width, 
    kMainViewY*height, 
    (kMainViewX+kMainViewW)*width, 
    (kMainViewY+kMainViewH)*height));
  gl::drawSolidRect(Rectf(kPreviewX*width,
    kPreviewY*height, 
    (kPreviewX+kPreviewW)*width, 
    (kPreviewY+kPreviewH)*height));
  gl::drawSolidRect(Rectf(kPaletteX*width,
    kPaletteY*height, 
    (kPaletteX+kPaletteW)*width, 
    (kPaletteY+kPaletteH)*height));

  gl::color(kLineColor);
  gl::drawStrokedRect(Rectf(kMainViewX*width,
    kMainViewY*height, 
    (kMainViewX+kMainViewW)*width, 
    (kMainViewY+kMainViewH)*height));
  gl::drawStrokedRect(Rectf(kPreviewX*width,
    kPreviewY*height, 
    (kPreviewX+kPreviewW)*width, 
    (kPreviewY+kPreviewH)*height));
  gl::drawStrokedRect(Rectf(kPaletteX*width,
    kPaletteY*height, 
    (kPaletteX+kPaletteW)*width, 
    (kPaletteY+kPaletteH)*height));

  //palette and preview title bars
  gl::color(kTitlebarColor);
  gl::drawSolidRect(Rectf(kPaletteX*width,
    kPaletteY*height, 
    (kPaletteX+kPaletteW)*width, 
    kPaletteY*height + kLabelH ));
  gl::drawSolidRect(Rectf(kPreviewX*width,
    kPreviewY*height, 
    (kPreviewX+kPreviewW)*width, 
    kPreviewY*height + kLabelH ));
  gl::color(kLineColor);
  gl::drawStrokedRect(Rectf(kPaletteX*width,
    kPaletteY*height, 
    (kPaletteX+kPaletteW)*width, 
    kPaletteY*height + kLabelH ));
  gl::drawStrokedRect(Rectf(kPreviewX*width,
    kPreviewY*height, 
    (kPreviewX+kPreviewW)*width, 
    kPreviewY*height + kLabelH ));

  //palette and preview labels
  ci::Vec2i palette_pos((kPaletteX + .5 * kPaletteW) * width - palette_label_.getWidth() * 0.5f ,
    kPaletteY * height - kTextShift);
  ci::Vec2i preview_pos((kPreviewX + .5 * kPreviewW) * width-preview_label_.getWidth() * 0.5f ,
    kPreviewY * height - kTextShift);


  gl::color(1.0,1.0,1.0,1.0);
  gl::draw(palette_label_, palette_pos);
  gl::draw(preview_label_, preview_pos);  
}
void PixUI::DrawWeightingMode() {
  Rectf img_area = GetImgPos();
  float scale = (img_area.x2 - img_area.x1)/input_img_.getWidth();

  glPushMatrix();
  //draw input image
  glTranslatef(img_area.x1,img_area.y1,0.0f);
  glScalef(scale, scale, 1.0f);
  gl::color(1.0f,1.0f,1.0f,1.0f);
  gl::draw(input_img_, input_img_.getBounds());
  gl::draw(weight_img_, weight_img_.getBounds());
  glPopMatrix();

  ci::Vec2i mouse_pos = getMousePos();
  if(GetImgPos().contains(mouse_pos))
  {
    gl::color(brush_weight_,brush_weight_,brush_weight_,.5f);
    gl::drawSolidCircle(mouse_pos, brush_size_*scale);
    gl::color(0.0f,1.0f,0.0f);
    gl::drawStrokedCircle(mouse_pos, brush_size_*scale);
  }
  std::stringstream stream;
  stream << "Weighting: Optional Step. Use the brush to mark areas as ";
  stream << "important (weight = 1) or unimportant (weight = 0). Click ";
  stream << "Process when done.";
  set_message(stream.str());
}
void PixUI::DrawIteratingMode()
{
  Rectf r = GetImgPos();
  float scale = (r.x2 - r.x1)/input_img_.getWidth();

  glPushMatrix();
  gl::color(1.0f,1.0f,1.0f,1.0f);
  glTranslatef(r.x1,r.y1,0.0f);
  glScalef(scale, scale, 1.0f); //scale to size of drawing area
  glScalef(input_img_.getWidth()/((float)output_img_.getWidth()),
    input_img_.getHeight()/((float)output_img_.getHeight()),
    1.0f); //upscale output img
  gl::draw(output_img_, output_img_.getBounds());

  glPopMatrix();

  DrawPalette();
  DrawPreview();

  std::stringstream stream;
  stream <<"Processing: ";
  stream <<"Itr: ";
  stream <<pix_->get_iteration();
  stream <<" Colors: ";
  stream <<pix_->GetPalette().size();
  set_message(stream.str());
}
void PixUI::DrawEditMode()
{
  Rectf img_area = GetImgPos();
  gl::color(1.0f,1.0f,1.0f,1.0f);
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
  glPushMatrix();

  glTranslatef(img_area.x1,img_area.y1,0.0f);
  float input_scale = (img_area.x2 - img_area.x1)/input_img_.getWidth();
  glScalef(input_scale, input_scale, 1.0f);
  //draw input image
  gl::draw(input_img_, input_img_.getBounds());

  glScalef(input_img_.getWidth()/((float)output_img_.getWidth()),
    input_img_.getHeight()/((float)output_img_.getHeight()),
    1.0f);
  if(superpixels_are_visible_)
  {
    //draw superpixel means
    gl::draw(average_img_, average_img_.getBounds());
  }
  else
  {
    //draw output image
    if(!output_is_hidden_)
    {
      gl::color(1.0f,1.0f,1.0f,1.0f-transparency_);	
      gl::draw(output_img_, output_img_.getBounds());
    }
    //draw fixed pixels
    if(fixed_are_visible_){
      glLineWidth(2.0f);
      gl::color(1.0f,1.0f,0.0f,1.0f);
      std::vector<std::list<int>> lp = pix_->get_pixel_constraints();
      for(int i = 0; i< output_img_.getWidth(); ++i){
        for(int j = 0; j < output_img_.getHeight(); ++j){
          int num_constraints = lp[pix_->vec2idx(cv::Vec2i(i,j))].size();

          if(num_constraints > 0){
            gl::drawStrokedRect(Rectf(i, j, i+1, j+1));
          }
        }
      }		
    }

  }
  //draw brush
  std::list<cv::Vec2i> pixels = GetPixelBrush(GetImgClick(getMousePos(), 
    output_width_, 
    output_height_));
  gl::color(0.0f,1.0f,0.0f);
  for(std::list<cv::Vec2i>::iterator pxl = pixels.begin(); pxl != pixels.end(); ++pxl){
    int i = (*pxl)[0];
    int j = (*pxl)[1];
    gl::drawStrokedRect(Rectf(i, j, (i+1), (j+1)));
  }

  glPopMatrix();

  DrawPalette();
  DrawPreview();

  std::stringstream stream;
  stream << "Editting: Select colors from the palette to ";
  stream << "change or paint onto the picture.";
  stream << " Click Process to run the algorithm using your changes.";
  set_message(stream.str());
}
void PixUI::DrawPalette()
{
  std::vector<cv::Vec3f> palette = pix_->GetPalette();
  std::vector<bool> locks = pix_->get_locked_colors();

  for(int i = 0; i< palette.size(); ++i) {
    Rectf color_area = GetPalettePos(i);
    float len = color_area.x2-color_area.x1;

    gl::color(palette[i][0], palette[i][1], palette[i][2]);
    gl::drawSolidRect(color_area, false);

    glPushMatrix();
    gl::translate(color_area.x1,color_area.y1,0.0f);

    //determine lock and selection color based on color brightness
    if(rgb2lab(palette[i])[0] > 50)
      gl::color(kBrightColor);
    else gl::color(kDarkColor);

    //draw lock
    if(locks[i]) {
      glPushMatrix();
      gl::scale(len/lock_img_.getWidth(), len/lock_img_.getHeight());
      gl::draw(lock_img_, lock_img_.getBounds());
      glPopMatrix();
    }
    //draw selection
    if(selected_colors[i]) {
      glPushMatrix();
      gl::scale(len/select_img_.getWidth(), len/select_img_.getHeight());
      gl::draw(select_img_, select_img_.getBounds());
      glPopMatrix();
    }
    glPopMatrix();
  }
}
void PixUI::DrawPreview() {
  if(state_ != EDIT && state_ != ITERATING) return;
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
  gl::color(1.0f,1.0f,1.0f);
  Rectf preview_area = GetPreviewPos();

  glPushMatrix();

  gl::translate(preview_area.x1, preview_area.y1);
  gl::draw(output_img_, output_img_.getBounds());

  glPopMatrix();
}
void PixUI::DrawMessage() {
  TextLayout message_text;
  message_text.setFont(font_);
  message_text.setColor(ColorA(1.0,1.0,1.0,1.0));
  message_text.addLine(message_);
  Surface8u message_rendered = message_text.render(true,false);
  gl::Texture message_texture(message_rendered);
  message_texture.setMagFilter(GL_NEAREST);
  message_texture.setMinFilter(GL_NEAREST);
  ci::Vec2i pos(kMainViewX*getWindowWidth(), getWindowHeight() - 30);
  gl::color(1.0,1.0,1.0,1.0);
  gl::draw(message_texture, pos);
}
void PixUI::set_message(std::string message){
  message_ = message;
}
void PixUI::SetState(STATE s) {
  if(state_ == s) return;
  state_ = s;
  UpdateBar();
}
Rectf PixUI::GetImgPos() {
  float img_ratio = input_img_.getAspectRatio();
  float width = kMainViewW*getWindowWidth();
  float height = kMainViewH*getWindowHeight();
  float area_ratio = width/height;
  float scale;
  if(img_ratio >= area_ratio) {
    scale = width/((float)input_img_.getWidth());
  } else {
    scale = height/((float)input_img_.getHeight());
  }
  float img_width = scale*input_img_.getWidth();
  float img_height = scale*input_img_.getHeight();

  float x1 = (getWindowWidth()*kMainViewX + .5f*(width - img_width));
  float x2 = x1 + img_width;
  float y1 = (getWindowHeight()*kMainViewY + .5f*(height - img_height));
  float y2 = y1 + img_height;

  return Rectf(x1,y1,x2,y2);
}
Rectf PixUI::GetPreviewPos() {
  float img_ratio = output_img_.getAspectRatio();
  float width = kPreviewW*getWindowWidth();
  float height = kPreviewH*getWindowHeight() - kLabelH;
  float area_ratio = width/height;
  float img_width = output_img_.getWidth();
  float img_height = output_img_.getHeight();

  int x1 = (int) (getWindowWidth()*kPreviewX + .5f*(width - img_width));
  int x2 = x1 + (int)img_width;
  int y1 = (int) (getWindowHeight()*kPreviewY + kLabelH 
    + .5f*(height - img_height));
  int y2 = y1 + (int)img_height;

  return Rectf(x1, y1, x2, y2);
}
Rectf PixUI::GetPalettePos(int idx) {
  float width = kPaletteW*getWindowWidth();
  float height = kPaletteH*getWindowHeight()- kLabelH;	
  float length = sqrt(width*height/pix_->get_max_palette_size());
  int num_wide = (int)ceil(width/length);
  int num_high = (int)floor(height/length);
  if(num_wide*num_high < pix_->get_max_palette_size()) {
    num_high = (int)ceil(height/length);
  }

  length = std::min(width/num_wide, height/num_high);
  int pos_x = idx % num_wide;
  int pos_y = (int)floor(((float)idx) / num_wide);

  float x1 = length * pos_x + .5f*(width-num_wide*length) 
    + kPaletteX*getWindowWidth();
  float x2 = x1 + length;
  float y1 = length*pos_y + .5f*(height-num_high*length) 
    + kPaletteY*getWindowHeight() + kLabelH;
  float y2 = y1 + length;

  return Rectf(x1,y1,x2,y2);
}
cv::Vec2i PixUI::GetImgClick(ci::Vec2i click, int w, int h) {
  Rectf img_area = GetImgPos();

  cv::Vec2i pixel = cv::Vec2i(click.x,click.y) - 
    cv::Vec2i(img_area.x1, img_area.y1);
  pixel[0] *= w / (img_area.x2 - img_area.x1);
  pixel[1] *= h / (img_area.y2 - img_area.y1);

  return pixel;
}
int PixUI::GetPaletteClick(ci::Vec2i click) {
  for(int i = 0; i<pix_->get_max_palette_size(); ++i) {
    Rectf color_area = GetPalettePos(i);
    if(color_area.contains(click))
      return i;
  }
  return -1;
}
void PixUI::PaintPixels() {
  if(paint_queue_.empty()) return;
  while(!paint_queue_.empty()) {
    std::pair<cv::Vec2i, MOUSECLICK> next = paint_queue_.front();
    paint_queue_.pop();
    std::list<int> con;

    if(next.second == LEFT)
      con = selected_list;

    std::list<cv::Vec2i>  area = GetPixelBrush(next.first);
    for(std::list<cv::Vec2i>::iterator n = area.begin(); n != area.end(); ++n) {
      pix_->SetPixelConstraints(*n, con);
    }
  }

  pix_->AssociatePalette();
  output_img_is_outdated_ = true;

}
void PixUI::PaintWeight() {
  if(paint_queue_.empty()) return;
  float d = brush_size_*brush_size_;
  while(!paint_queue_.empty()) {
    cv::Vec2i c = paint_queue_.front().first;
    paint_queue_.pop();

    int x1 = std::max((int)(c[0] - brush_size_), 0);
    int x2 = std::min((int)(c[0] + brush_size_),input_img_.getWidth()-1);
    int y1 = std::max((int)(c[1] - brush_size_), 0);
    int y2 = std::min((int)(c[1] + brush_size_), input_img_.getHeight()-1);

    for(int i = x1; i<= x2; ++i) {
      for(int j = y1; j<= y2; ++j) {
        if((i-c[0])*(i-c[0]) + (j-c[1])*(j-c[1]) <= d) {
          input_weights.at<float>(j,i) = brush_weight_;
        }
      }
    }
  }
  weight_img_is_outdated_ = true;
}
std::list<cv::Vec2i> PixUI::GetPixelBrush(cv::Vec2i center) {
  std::list<cv::Vec2i> pixels;

  //if center of brush is out of the drawing area, return empty list
  if(center[0] < 0 || center[0] >= output_img_.getWidth()
    || center[1] < 0 || center[1] >= output_img_.getHeight())
    return pixels;

  for(int i = -brush_size_; i<= brush_size_; ++i) {
    for(int j = -brush_size_; j<= brush_size_; ++j) {
      if( i*i + j*j <= brush_size_*brush_size_ -.5f  
        && center[0]+i >= 0 
        && center[0]+i < output_img_.getWidth() 
        && center[1]+j >= 0 && center[1]+j < output_img_.getHeight()){
          pixels.push_back(cv::Vec2i(center[0]+i,center[1]+j));
      }
    }
  }
  return pixels;
}
void PixUI::SetColor() {
  if(selected_list.size() != 1) return;
  int index = selected_list.front();
  pix_->SetColor(index, cv::Vec3f(color_picker_.r, 
    color_picker_.g, 
    color_picker_.b)); 
  Reassociate();
}
void PixUI::UpdateWeightImg(){
  cv::Mat w(input_weights.size(), CV_8UC4);
  for(int i = 0; i< w.cols; ++i) {
    for(int j = 0; j<w.rows; ++j) {
      w.at<cv::Vec4b>(j,i) = 
        cv::Vec4b(127,127,127, 255*(1.0f-input_weights.at<float>(j,i)));
    }
  }
  weight_img_ = fromOcv(w);
}
void PixUI::NewProj() {
  std::vector<std::string> exts;
  exts.push_back("png");
  exts.push_back("jpg");
  exts.push_back("bmp");
  exts.push_back("gif");

  std::string filename = OpenFileDialog(exts);
  if(filename.empty()) return;
  try {
    input_img_ = gl::Texture(loadImage(filename));
    SetState(NODATA);
    Reset();
  } catch(...) {
    return;
  }	

}
void PixUI::LoadProj() {
  std::vector<std::string> exts;
  exts.push_back("pix");
  std::string filename = OpenFileDialog(exts);
  if(filename.empty()) return;
  try {
    Pix * next = new Pix(filename);
    if(pix_)
      delete pix_;
    pix_ = next;
    cv::Mat in;
    pix_->GetInputImage(in);
    input_img_ = fromOcv(in);
    selected_colors = std::vector<bool>(pix_->get_max_palette_size(), false);
    selected_list.clear();
    output_width_ = pix_->get_output_width();
    output_height_ = pix_->get_output_height();
    max_palette_size_ = pix_->get_max_palette_size();
    SetState(EDIT);
    output_img_is_outdated_ = true;
  } catch(...) {
    return;
  }	
}
void PixUI::Reset() {

  if(state_ == NODATA || state_ == WEIGHTING) {
    input_weights = 
      cv::Mat(cv::Size(input_img_.getWidth(), input_img_.getHeight()), 
      CV_32FC1, cv::Scalar(1.0f));
    UpdateWeightImg();
  }

  if(state_ != WEIGHTING) {
    if(pix_) delete pix_;
    pix_ = NULL;
    SetState(WEIGHTING);
    output_width_ = output_height_*input_img_.getAspectRatio()+.5f;
    previous_height_ = output_height_;
    previous_width_ = output_width_;
    brush_size_ = 10.0f;
  }




}
void PixUI::SaveProj() {
  if(pix_ == NULL) return;

  std::vector<std::string> exts;
  exts.push_back("pix");
  std::string filename = SaveFileDialog(exts);
  if(filename.empty()) return;
  try {
    pix_->SaveToFile(filename);		
  } catch(...) {
    return;
  }	
}
void PixUI::SaveImg() {
  if(pix_ == NULL) return;
  std::vector<std::string> exts;
  exts.push_back("png");
  std::string filename = SaveFileDialog(exts);

  if(filename.empty()) return;
  try {
    writeImage(filename, output_img_);	
  } catch(...) {
    return;
  }	
}
std::string PixUI::OpenFileDialog(std::vector<std::string> exts) {
  return getOpenFilePath( "", exts).string();
}
std::string PixUI::SaveFileDialog(std::vector<std::string> exts) {
  std::string path = getSaveFilePath("",exts).string();
  if(! path.empty() && exts.size() > 0 &&  !endsWith(path,exts)) {
    path += "." + exts[0];
  }
  return path;
}

CINDER_APP_BASIC(PixUI, RendererGl )
