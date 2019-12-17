#include "camera_pipeline.hpp"

std::unique_ptr<Image<RgbPixel>> CameraPipeline::ProcessShot() const {
    
  // BEGIN: CS348K STUDENTS MODIFY THIS CODE

  // put the lens cap on if you'd like to measure a "dark frame"
  sensor_->SetLensCap(true);

  //to use dark frame result as the basis for correction    
  // grab RAW pixel data from sensor
  const int width = sensor_->GetSensorWidth();
  const int height = sensor_->GetSensorHeight();
  auto raw_data_dark = sensor_->GetSensorData(0, 0, width, height);

  //now turn on the camera by removing the lenscap
  sensor_->SetLensCap(false);
  auto raw_data = sensor_->GetSensorData(0, 0, width, height);
    
  // In this function you should implement your full RAW image processing pipeline.
  //   (1) Demosaicing
  //   (2) Address sensing defects such as bad pixels and image noise.
  //   (3) Apply local tone mapping based on the local laplacian filter or exposure fusion.
  //   (4) gamma correction
    
  // allocate 3-channel RGB output buffer to hold the results after processing 
  std::unique_ptr<Image<RgbPixel>> image(new Image<RgbPixel>(width, height));

  // allocate 3-channel YUV output buffer to hold the results after processing 
  std::unique_ptr<Image<YuvPixel>> yuvimage(new Image<YuvPixel>(width, height));
  
  // The starter code copies the raw data from the sensor to all rgb
  // channels. This results in a gray image that is just a
  // visualization of the sensor's contents.

  // > The Bayer filter pattern has green at (0,0)
  // > then alternates R/G along row
  // > and alternates B/G along column

  // > So positions with green are (2*i, 2*j) and (2*i+1, 2*j+1)
  // > other places green can be interpolated with the neighbours
  // > positions with blue are (2*i+1, 2*j) like (1,0), (1,2), etc.
  // > positions with red are (2*i, 2*j+1) like (0,1), (0,3), etc.
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      const auto val = raw_data->data(row, col) - raw_data_dark->data(row, col);
      auto& pixel = (*image)(row, col);

      // pixel data from the sensor is normalized to the 0-1 range, so
      // scale by 255 for the final image output.  Output image pixels
      // should be in the 0-255 range.
      /* pixel.r = val * 255.f;
      pixel.g = val * 255.f;
      pixel.b = val * 255.f; */

      if(row%2==1 && col%2==1)  // green filter is active/valid
      {
        pixel.g += val * 255.f;

        //blue filter is to left and right, if those are valid locs
        //init counts and total
        float totalb = 0;
        int bctr = 0;
        if(col-1 > 0)
        {
          bctr += 1;
          totalb += raw_data->data(row, col-1)-raw_data_dark->data(row, col-1);
          if(row+2 < height)
          {
            bctr += 1;
            totalb += raw_data->data(row+2, col-1)-raw_data_dark->data(row+2, col-1);
          }
        }
        if(col+1 < width)
        {
          bctr += 1;
          totalb += raw_data->data(row, col+1)-raw_data_dark->data(row, col+1);
          if(row+2 < height)
          {
            bctr += 1;
            totalb += raw_data->data(row+2, col+1)-raw_data_dark->data(row+2, col+1);
          }
        }
        pixel.b += totalb/bctr * 255.f;

        float totalr = 0;
        int rctr = 0;
        if(row-1 > 0)
        {
          rctr += 1;
          totalr += raw_data->data(row-1, col)-raw_data_dark->data(row-1, col);
          if(col+2 < width)
          {
            rctr += 1;
            totalr += raw_data->data(row-1, col+2)-raw_data_dark->data(row-1, col+2);
          }
        }
        if(row+1 < height)
        {
          rctr += 1;
          totalr += raw_data->data(row+1, col)-raw_data_dark->data(row+1, col);
          if(col+2 < width)
          {
            rctr += 1;
            totalr += raw_data->data(row+1, col+2)-raw_data_dark->data(row+1, col+2);
          }
        }
        pixel.r += totalr/rctr * 255.f;

      }  
      else if(row%2==0 && col%2==1)   // meaning red filter is active/valid
      {
        pixel.r += val * 255.f;

        //calculate g value from left, right, top, bottom if they exist
        int gctr = 0; int bctr=0;
        float totalg = 0; int totalb=0;
        if(row-1 > 0)
        {
          gctr+=1; totalg+=raw_data->data(row-1, col)-raw_data_dark->data(row-1, col);
          if(col-1 > 0)
          {
            bctr+=1; totalb+=raw_data->data(row-1, col-1)-raw_data_dark->data(row-1, col-1);
          }
          if(col+1 < width)
          {
            bctr+=1; totalb+=raw_data->data(row-1, col+1)-raw_data_dark->data(row-1, col+1);
          }
        }
        if(row+1 < height)
        {
          gctr+=1; totalg+=raw_data->data(row+1, col)-raw_data_dark->data(row+1, col);
          if(col-1 > 0)
          {
            bctr+=1; totalb+=raw_data->data(row+1, col-1)-raw_data_dark->data(row+1, col-1);
          }
          if(col+1 < width)
          {
            bctr+=1; totalb+=raw_data->data(row+1, col+1)-raw_data_dark->data(row+1, col+1);
          }
        }
        if(col-1 > 0)
        {
          gctr+=1; totalg+=raw_data->data(row, col-1)-raw_data_dark->data(row, col-1);
        }
        if(col+1 < width)
        {
          gctr+=1; totalg+=raw_data->data(row, col+1)-raw_data_dark->data(row, col+1);
        }
        pixel.g += totalg/gctr * 255.f;
        pixel.b += totalb/bctr * 255.f;
      }
      else if(col%2==0 && row%2==1)  // meaning blue filter is active/valid
      {
        pixel.b += val * 255.f;

        //calculate r and g value from left, right, top, bottom if they exist
        int gctr = 0; int rctr=0;
        float totalg = 0; int totalr=0;
        if(row-1 > 0)
        {
          gctr+=1; totalg+=raw_data->data(row-1, col)-raw_data_dark->data(row-1, col);
          if(col-1 > 0)
          {
            rctr+=1; totalr+=raw_data->data(row-1, col-1)-raw_data_dark->data(row-1, col-1);
          }
          if(col+1 < width)
          {
            rctr+=1; totalr+=raw_data->data(row-1, col+1)-raw_data_dark->data(row-1, col+1);
          }
        }
        if(row+1 < height)
        {
          gctr+=1; totalg+=raw_data->data(row+1, col)-raw_data_dark->data(row+1, col);
          if(col-1 > 0)
          {
            rctr+=1; totalr+=raw_data->data(row+1, col-1)-raw_data_dark->data(row+1, col-1);
          }
          if(col+1 < width)
          {
            rctr+=1; totalr+=raw_data->data(row+1, col+1)-raw_data_dark->data(row+1, col+1);
          }
        }
        if(col-1 > 0)
        {
          gctr+=1; totalg+=raw_data->data(row, col-1)-raw_data_dark->data(row, col-1);
        }
        if(col+1 < width)
        {
          gctr+=1; totalg+=raw_data->data(row, col+1)-raw_data_dark->data(row, col+1);
        }
        pixel.g += totalg/gctr * 255.f;
        pixel.r += totalr/rctr * 255.f;
      }
      else  // neither row nor column are divisible by 2 so green active/valid
      {
        pixel.g += val * 255.f;

        //red filter is to left and right, if those are valid locs
        //init counts and total
        float totalr = 0;
        int rctr = 0;
        if(col-1 > 0)
        {
          rctr += 1;
          totalr += raw_data->data(row, col-1)-raw_data_dark->data(row, col-1);
          if(row+2 < height)
          {
            rctr += 1;
            totalr += raw_data->data(row+2, col-1)-raw_data_dark->data(row+2, col-1);
          }
        }
        if(col+1 < width)
        {
          rctr += 1;
          totalr += raw_data->data(row, col+1)-raw_data_dark->data(row, col+1);
          if(row+2 < height)
          {
            rctr += 1;
            totalr += raw_data->data(row+2, col+1)-raw_data_dark->data(row+2, col+1);
          }
        }
        pixel.r += totalr/rctr * 255.f;

        float totalb = 0;
        int bctr = 0;
        if(row-1 > 0)
        {
          bctr += 1;
          totalb += raw_data->data(row-1, col)-raw_data_dark->data(row-1, col);
          if(col+2 < width)
          {
            bctr += 1;
            totalb += raw_data->data(row-1, col+2)-raw_data_dark->data(row-1, col+2);
          }
        }
        if(row+1 < height)
        {
          bctr += 1;
          totalb += raw_data->data(row+1, col)-raw_data_dark->data(row+1, col);
          if(col+2 < width)
          {
            bctr += 1;
            totalb += raw_data->data(row+1, col+2)-raw_data_dark->data(row+1, col+2);
          }
        }
        pixel.b += totalb/bctr * 255.f;
      }
    }
  }

  // Get the YUV image and blur the U/V channels (luminance and chrominance)
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      auto& rgbpixel = (*image)(row, col);
      auto& yuvpixel = (*yuvimage)(row,col);
      yuvpixel = rgbpixel.RgbToYuv(rgbpixel);
    }
  }
  
  // return processed image output
  return yuvimage;

  // END: CS348K STUDENTS MODIFY THIS CODE  
}
