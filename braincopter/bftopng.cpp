
/*
Converts a brainfuck file + any image into braincopter code encoded in that image (all images are .png).

Compiles with "g++ lodepng.cpp bftopng.cpp -o bftopng"
*/
#include "lodepng.h"

#include <vector>

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

int avIntRGB(int color)
{
    int R = (color / 65536) % 256;
    int G = (color / 256) % 256;
    int B = color % 256;
    return (R + G + B) / 3;
}

#define NUMCOMMANDS 11
void setImagePixel(unsigned char* sourceimg, unsigned char* image, int x, int y, int iw, int sh, int command)
{
    int pixel;
    
    int R = sourceimg[4 * iw * y + 4 * x + 0];
    int G = sourceimg[4 * iw * y + 4 * x + 1];
    int B = sourceimg[4 * iw * y + 4 * x + 2];
    if(y < sh) pixel = 65536 * R + 256 * G + B;
    else pixel = 0;

    pixel /= NUMCOMMANDS; //there are 11 commands
    pixel *= NUMCOMMANDS; //now it's modulo 11 is 0
    pixel += command; //now it's modulo 11 is command
    
    int alt1 = pixel;
    int alt2 = pixel + NUMCOMMANDS;
    int alt3 = pixel - NUMCOMMANDS;
    if(alt3 < 0) alt3 = alt1;
    if(alt2 >= 16777216) alt2 = alt1;
    int alt1diff = (R+G+B) / 3 - avIntRGB(alt1); if(alt1diff < 0) alt1diff = -alt1diff;
    int alt2diff = (R+G+B) / 3 - avIntRGB(alt2); if(alt2diff < 0) alt2diff = -alt2diff;
    int alt3diff = (R+G+B) / 3 - avIntRGB(alt3); if(alt3diff < 0) alt3diff = -alt3diff;
    int pixeldiff = alt1diff; if(pixeldiff < 0) pixeldiff = -pixeldiff;
    if(alt2diff <= pixeldiff) pixel = alt2;
    pixeldiff = (R+G+B) / 3 - avIntRGB(pixel); if(pixeldiff < 0) pixeldiff = -pixeldiff;
    if(alt3diff < pixeldiff) pixel = alt3;
    
    image[3 * iw * y + 3 * x + 0] = pixel / 65536;
    image[3 * iw * y + 3 * x + 1] = (pixel / 256) % 256;
    image[3 * iw * y + 3 * x + 2] = pixel % 256;
}

int main(int argc, char *argv[]) 
{
    if(!argv[1] || !argv[2])
    {
        cout << "please specify a brainfuck and a png file\n";
        return 0;
    }

    std::vector<unsigned char> file;
    LodePNG::loadFile(file, argv[1]);
    std::string code;
    for(size_t i = 0; i < file.size(); i++)
    {
      char c = file[i];
      if(c == '<' || c == '>' || c == '+' || c == '-' || c == '[' || c == ']' || c == '.' || c == ',') code += c;
    }
    int size = code.size();
    if(size < 1) std::cout << "error in input file" << std::endl;

    std::vector<unsigned char> sourceimg;
    unsigned w, h;
    int error = LodePNG::decode(sourceimg, w, h, argv[2]);
    if(error < 0)
    {
        cout << "png decoder sais: error " << error << "\n";
        return -1;
    }

    
    //generate image data
    int finalcodesize = code.size();
    int iw = int(w);
    int ih = code.size() / int(w) + 1 + 4; //4 extra lines represent the 2 extra colums, if ih is too large compared to iw this will actually crash
    
    

    unsigned char* image = new unsigned char[iw * ih * 3];
    
    int x = 0, y = 0;

    for(int i = 0; i < code.size(); i++)
    {
        bool nop = false;
        switch(code[i])
        {
            case '>':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 0);
                break;
            case '<':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 1);
                break;
            case '+':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 2);
                break;
            case '-':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 3);
                break;
            case '.':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 4);
                break;
            case ',':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 5);
                break;
            case '[':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 6);
                break;
            case ']':
                setImagePixel(&sourceimg[0], image, x, y, iw, ih, 7);
                break;
            default:
                nop = true;
                break;
        }
        
        if(!nop)
        {
            if(y % 2 == 0)
            {
                x++;
                if(x == iw - 1)
                {
                    //x++;
                    setImagePixel(&sourceimg[0], image, x, y, iw, ih, 8);
                    y++;
                    setImagePixel(&sourceimg[0], image, x, y, iw, ih, 8);
                    x--;
                }
            }
            else
            {
                x--;
                if(x == 0)
                {
                    //x--;
                    setImagePixel(&sourceimg[0], image, x, y, iw, ih, 9);
                    y++;
                    setImagePixel(&sourceimg[0], image, x, y, iw, ih, 9);
                    x++;
                }
            }
        }
    }

    //now fill the rest of the last line with nops
    if(y % 2 == 0)
    while(x < iw)
    {
        setImagePixel(&sourceimg[0], image, x, y, iw, ih, 10); x++;
    }
    else
    while(x >= 0)
    {
        setImagePixel(&sourceimg[0], image, x, y, iw, ih, 10); x--;
    }
    
    LodePNG::encode("result.png", image, iw, ih, 2, 8);
    return 0;
}

/*
Copyright (c) 2005 by Lode Vandevenne.
All rights reserved.
*/
