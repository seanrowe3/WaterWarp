// water.cpp written by Sean Rowe
// 12-3-14
// water.cpp creates a new image with a water reflection effect 

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <OpenImageIO/imageio.h>
#include <GL/glut.h>
#include <fstream>
#include <iostream>

using namespace std;
OIIO_NAMESPACE_USING;

ImageSpec spec;
int xres, yres;
char name[24];

struct RGBpixel { // 3 channel RGB structure
   float r;
   float g;
   float b;
};
struct RGBApixel{ // 4 channel RGBA structure
   float r;
   float g;
   float b;
   float a;
};

RGBpixel **out;
RGBpixel **inmap;
RGBpixel **newout;
RGBApixel **out4;
RGBApixel **inmap4;
RGBApixel **newout4;

// OpenGL DrawImage function
void DrawImage(){
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glRasterPos2i(0, yres);
   glPixelZoom(1, -1);
   if (spec.nchannels == 3){
      glDrawPixels(xres, yres, GL_RGB, GL_FLOAT, out[0]);
   }
   if (spec.nchannels == 4){
      glDrawPixels(xres, yres, GL_RGBA, GL_FLOAT, out4[0]);
   }
   glFlush(); 
}

// OpenGL save function 
void Save(){
   ImageOutput *output = ImageOutput::create (name);
   if (!output ) return;
   if (spec.nchannels == 3){
      ImageSpec spec(xres, yres, 3, TypeDesc::FLOAT);
      output->open(name, spec);
      output->write_image(TypeDesc::FLOAT, out[0]);
   }
   if (spec.nchannels == 4){
      ImageSpec spec(xres, yres, 4, TypeDesc::FLOAT);
      output->open(name, spec);
      output->write_image(TypeDesc::FLOAT, out4[0]);
   }
   output->close();
   //delete out;
}

// Copies the output arrays into the array that is displayed 
void Copy(){
   int row, col;

   if (spec.nchannels == 3){
      for (row = 0; row < yres; row++){
         for (col = 0; col < xres; col++){
            out[row][col] = newout[row][col];
         }
      }
   }

   if (spec.nchannels == 4){
      for (row = 0; row < yres; row++){
         for (col = 0; col < xres; col++){
            out4[row][col] = newout4[row][col];
         }
      }  
   }
}

// Reshapes the opengl window for the new image
void Reshape(){

   glutReshapeWindow(xres, yres);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, xres, 0, yres);
   glutPostRedisplay(); 

} 

// Tonemap computes a basic tone mapping operation on the lower half of the
void Tonemap(){
   int row, col; 
   float lw, ld, cd, r, g, b;
   float G = 3;

   for (row = (yres/2) - 5; row < yres; row++){
      for (col = 0; col < xres; col++){

         r = out[row][col].r;
         g = out[row][col].g;
         b = out[row][col].b;
         
         lw = (1/61.0)*((20*r)+(40*g)+b); // determine luminance for each pixel
         ld = exp(G * log(lw));
         cd = (ld/lw); // tonemap new output with this luminance
         newout[row][col].r = out[row][col].r;
         newout[row][col].b = out[row][col].b;
         newout[row][col].g = out[row][col].g;
      }
   }
   Copy();
}

// Applies a gaussian blur to the lower half of the image
void Blur(){
   int row, col, W, x, y;
   float sigma, mean, sum, r, g, b, mag;
   sigma = 5;
   float kernel[5][5];
   sum = 0.0;
   mean = 5/2;
   // calculates the gaussian blur filter
   for (row = 0; row < 5; row++){
      for (col = 0; col < 5; col++){
         kernel[row][col] = exp(-0.5 * (pow(((float)row-mean)/sigma, 2.0) + pow(((float)col-mean)/sigma, 2.0))) / (2 * M_PI * sigma *sigma);
         sum += kernel[row][col];
      }
   }
   // normalizes the gaussian blur filter
   for (row = 0; row < 5; row++){
      for (col = 0; col < 5; col++){
         kernel[row][col] /= sum;
      }
   }
   // computes filter operations on lower half of the image 
   for (row = (yres/2) - 5; row < yres; row++){
      for (col = 0; col < xres; col++){
         if (spec.nchannels == 3){
            r = 0.0; g = 0.0; b = 0.0;
            for (x = -2; x < 3; x++){
               for (y = -2; y < 3; y++){
                  if(!(col < 3 || col > xres-2 || row >= yres-2)){
                    r += kernel[x+2][y+2] * out[row+y][col+x].r;  
                    g += kernel[x+2][y+2] * out[row+y][col+x].g;
                    b += kernel[x+2][y+2] * out[row+y][col+x].b;
                  }
               } // y < 5 (kernel cols)
            } // x < 5 (kernel rows)
            newout[row][col].r = r;
            newout[row][col].b = b;
            newout[row][col].g = g;
         }// 3 channel image
         if (spec.nchannels == 4){
            r = 0.0; g = 0.0; b = 0.0;
            for (x = -2; x < 3; x++){
               for (y = -2; y < 3; y++){
                  if(!(col < 3 || col > xres-2 || row >= yres-2)){
                    r += kernel[x+2][y+2] * out4[row+y][col+x].r;  
                    g += kernel[x+2][y+2] * out4[row+y][col+x].g;
                    b += kernel[x+2][y+2] * out4[row+y][col+x].b;
                  }
               } // y < 5 (kernel cols)
            } // x < 5 (kernel rows)
            newout4[row][col].r = r;
            newout4[row][col].b = b;
            newout4[row][col].g = g;
         }// 4 channel image
      } // col < xres
   } // row < yres

   Copy();
}

// Flip vertically flips the image 
void Flip(){
   int height, count, row, col, i, nrow;

   height = spec.height *2;

   count = 1;
   // setting up new output that has twice the height of original.
   if (spec.nchannels == 3){
      // reallocation for new pixel array 
      newout = new RGBpixel*[height];
      newout[0] = new RGBpixel[spec.width * height];
      out = new RGBpixel*[height];
      out[0] = new RGBpixel[spec.width * height];
      for (i = 1; i < height; i++){
         newout[i] = newout[i-1] + spec.width;
         out[i] = out[i-1] +spec.width;
      }

      nrow = height; // nrow counts backwards to find the column 
      for (row = 0; row <= height; row++){
         nrow -= 1;
         for (col = 0; col <= spec.width; col++){
            if (row < spec.height) newout[row][col] = inmap[row][col];
            else {
               if (nrow >= 0 && row < height) newout[row][col] = inmap[nrow][col];   
            } // row > spec.height
         }// for col 
      }// for row 
   } // spec.nchannels = 3
   if (spec.nchannels == 4){
      // reallocation for new pixel array 
      newout4 = new RGBApixel*[height];
      newout4[0] = new RGBApixel[spec.width * height];
      out4 = new RGBApixel*[height];
      out4[0] = new RGBApixel[spec.width * height];
      for (i = 1; i < height; i++){
         newout4[i] = newout4[i-1] + spec.width;
         out4[i] = out4[i-1] +spec.width;
      }

      nrow = height; // nrow counts backwards to find the column 
      for (row = 0; row <= height; row++){
         nrow -= 1;
         for (col = 0; col <= spec.width; col++){
            if (row < spec.height) newout4[row][col] = inmap4[row][col];
            else {
               if (nrow >= 0 && row < height) newout4[row][col] = inmap4[nrow][col];   
            } // row > spec.height
         }// for col 
      }// for row 
   } // spec.nchannels = 4

	yres = height; // reset the yres 
   Copy(); 
   Reshape();

}

// Water creates the wave effect on the lower half using a sine wave eqution
void Water(){
   int row, col, y, count;
   bool cycle;
   float rowf, colf;
   float n;

   Flip();

   cycle = true; 

   // calculates a sine wave warp or a cosine wave warp every 15 pixels 
   for (count = (yres/2) - 5; count < yres; count+=15){
      for (row = count; row < yres; row++){
         for (col = 0; col < xres; col++){
            rowf = (float)row;
            colf = (float)col;
            if (cycle)
               y = fmod((rowf + round((yres/1.5)*sin(colf/xres)) + count), count);
            else
               y = fmod((rowf + round((yres/1.5)*cos(colf/xres)) + count), count);
            if (spec.nchannels == 3)
               newout[row][col] = out[row][y];
            else 
               newout4[row][col] = out4[row][y];
         } // for col < xres
      } // for row < yres 
   } // count < yres  

   Copy();
   Blur();
   Tonemap();
   DrawImage();
}

// OpenGL Handle Keyboard strokes function
void HandleKey(unsigned char key, int x, int y){
   switch(key){
      case 'q': 
        exit(0);
      case 'w':
        Water();
      case 's':
        Save();
      default: 
         return;
   }  
}

int main(int argc, char *argv[]){
   ImageInput *in;
   int i, row, col;

   // read in the input image
   if (argc < 2) {
      cout << "Correct usage: ./water in.png [out.png]";
      exit(-1);
   }
   in = ImageInput::open(argv[1]);
   if (!in) cout << "Input image did not work\n";
   in->open(argv[1], spec);
   xres = spec.width;
   yres = spec.height;
   // Two separate ways to read in, one for 3 channel, one for 4 channel. 
   if (spec.nchannels == 3){
      // allocation of pixel arrays
      inmap = new RGBpixel*[spec.height];
      inmap[0] = new RGBpixel[spec.width * spec.height];
      out = new RGBpixel*[yres];
      out[0] = new RGBpixel[xres*yres];
      for(i = 1; i < spec.height; i++){
         inmap[i] = inmap[i-1] + spec.width;
         out[i] = out[i-1] + spec.width;
      }
      in->read_image(TypeDesc::FLOAT, inmap[0]);
      // copy the input into output array 
      for (row = 0; row < yres; row++){
         for (col = 0; col < xres; col++){
            out[row][col] = inmap[row][col];
         }
      }
   } // spec.channels = 3
   if (spec.nchannels == 4){
      // allocation of pixel arrays
      inmap4 = new RGBApixel*[spec.height];
      inmap4[0] = new RGBApixel[spec.width * spec.height];
      out4 = new RGBApixel*[yres];
      out4[0] = new RGBApixel[xres*yres];
      for (i = 1; i < spec.height; i++){
         inmap4[i] = inmap4[i-1] + spec.width;
         out4[i] = out4[i-1] +spec.width;
      }
      in->read_image(TypeDesc::FLOAT, inmap4[0]);
      // copy the input into output arry
      for (row = 0; row < yres; row++){
         for (col = 0; col < xres; col++){
            out4[row][col] = inmap4[row][col];
         }
      }
   } // spec.channels = 4
   in->close();
   if (argc == 3){
      strcpy(name, argv[2]);
   }

   // OpenGL display functions
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_RGBA);
   glutInitWindowSize(xres, yres);
   glutCreateWindow("New Image");

   glutDisplayFunc(DrawImage);
   glutKeyboardFunc(HandleKey);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, xres, 0, yres);
   glClearColor(1, 1, 1, 1);
   glutMainLoop(); 
   
   return 0;

}