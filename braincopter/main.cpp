//Compiles with "g++ lodepng.cpp main.cpp -O3 -o braincopter", manual at bottom

/*
Copyright (c) 2005 by Lode Vandevenne.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Lode Vandevenne nor the names of his contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "lodepng.h"

#include <vector>

#include <iostream>
#include <fstream>

using namespace std;
void moveIP(int &ipx, int &ipy, int dir)
{
    switch(dir)
    {
        case 0: ipx += 1; break;
        case 1: ipy += 1; break;
        case 2: ipx -= 1; break;
        case 3: ipy -= 1; break;
    }
}

void moveIPBack(int &ipx, int &ipy, int dir)
{
    switch(dir)
    {
        case 0: ipx -= 1; break;
        case 1: ipy -= 1; break;
        case 2: ipx += 1; break;
        case 3: ipy += 1; break;
    }
}

//rotate IP to the left
void rotright(int &dir)
{
    dir++;
    if(dir > 3) dir = 0;
}

//rotate IP to the right
void rotleft(int &dir)
{
    dir--;
    if(dir < 0) dir = 3;
}

int main(int argc, char *argv[]) 
{
    //read the file

    if(!argv[1])
    {
        cout << "please specify a file\n";
        return 0;
    }

    std::vector<unsigned char> image;
    
    unsigned w, h;

    int error = LodePNG::decode(image, w, h, argv[1]);
    if(error < 0)
    {
        cout << "png decoder sais: error " << error << "\n";
        return -1;
    }
    
    //convert the image code into intermediate code
    
    std::vector<unsigned char> code; //the converted code
    code.resize(w * h, 0);
    std::vector<int> jumpv[4]; //value belonging to this code (one for each direction)
    std::vector<int> jumpx[4]; //other x position belonging to this code (one for each direction)
    std::vector<int> jumpy[4]; //other y position belonging to this code (one for each direction)
    std::vector<int> jumpd[4]; //other direction belonging to this code (one for each direction)
    for(int i = 0; i < 4; i++)
    {
        jumpv[i].resize(w * h, -1);
        jumpx[i].resize(w * h, -1);
        jumpy[i].resize(w * h, -1);
        jumpd[i].resize(w * h, -1);
    }
    
    //fill in the new commands
    for(int y = 0; y < h; y++)
    for(int x = 0; x < w; x++)
    {
        int command = (65536 * image[y * w * 4 + x * 4 + 0] + 256 * image[y * w * 4 + x * 4 + 1] + image[y * w * 4 + x * 4 + 2]) % 11;
        switch(command)
        {
            case 0: code[y * w + x] = '>'; break;
            case 1: code[y * w + x] = '<'; break;
            case 2: code[y * w + x] = '+'; break;
            case 3: code[y * w + x] = '-'; break;
            case 4: code[y * w + x] = '.'; break;
            case 5: code[y * w + x] = ','; break;
            case 6: code[y * w + x] = '['; break;
            case 7: code[y * w + x] = ']'; break;
            case 8: code[y * w + x] = 'r'; break; //rotate right
            case 9: code[y * w + x] = 'l'; break; //rotate left
            default: code[y * w + x] = 0; break; //case 10
        }
    }
    
    //optimize (infinite loops cannot happen, the IP rotators are reversible and the IP starts at top left)
    for(int dir = 0; dir < 4; dir++)
    for(long l = 0; l < w * h; l++)
    {
        int x, y;
        if(dir == 0) x = l % w, y = l / w;
        else if(dir == 1) y = l % h, x = l / h;
        else if(dir == 2) x = w - l % w - 1, y = l / w;
        else if(dir == 3) y = h - l % h - 1, x = l / h;
        
        int command = command = code[w * y + x];
        int ipx, ipy, ipd, brackets;

        
        if(jumpv[dir][w * y + x] != -1) continue; //it's already filled in, no reason to do it again
        ipx = x, ipy = y, ipd = dir;
        
        if(command == '[')
        {
            int brackets = 0;
            bool done = 0;
            while(!done)
            {
                //3 end conditions: more than MAXROT rotation commands found (==> there is a chance for infinite loop), at end of file (end of code), or corresponding bracket found (only in the last case, actual values will be filled in)
                moveIP(ipx, ipy, ipd);
                
                if(ipx < 0 || ipx >= w || ipy < 0 || ipy >= h)
                {
                    done = true;
                }
                else
                {
                    command = code[w * ipy + ipx];
                    switch(command)
                    {
                        case '[': brackets++; break;
                        case ']': 
                            if(brackets <= 0)
                            {
                                done = true;
                                jumpx[dir][w * y + x] = ipx;
                                jumpy[dir][w * y + x] = ipy;
                                jumpd[dir][w * y + x] = ipd;
                                jumpx[ipd][w * ipy + ipx] = x;
                                jumpy[ipd][w * ipy + ipx] = y;
                                jumpd[ipd][w * ipy + ipx] = dir;
                            }
                            else brackets--;
                            break;
                        case 'r': rotright(ipd); break;
                        case 'l': rotleft(ipd); break;
                        default: break;
                    }
                }
            }
        }
        else if(command == '+' || command == '-' || command == '>' || command == '<' || command == 0)
        {
            int original = command; //the original command
            int value = 1;
            bool done = 0;
            while(!done)
            {
                //3 end conditions: more than MAXROT rotation commands found (==> there is a chance for infinite loop), at end of file (end of code), or corresponding bracket found (only in the last case, actual values will be filled in)
                moveIP(ipx, ipy, ipd);
                
                if(ipx < 0 || ipx >= w || ipy < 0 || ipy >= h)
                {
                    done = true;
                }
                else
                {
                    command = code[w * ipy + ipx];
                    if(command == original)
                    {
                        value++;
                        if(jumpv[ipd][w * ipy + ipx] == -1) jumpv[ipd][w * ipy + ipx] = -2; //indicate that this command is never needed anymore and must not be filled in by the next optimization loop
                    }
                    else if(command == 'r')
                    {
                        rotright(ipd); break;
                    }
                    else if(command == 'l')
                    {
                        rotleft(ipd); break;
                    }
                    else
                    {
                        if(value > 1)
                        {
                            moveIPBack(ipx, ipy, ipd);
                            jumpx[dir][w * y + x] = ipx;
                            jumpy[dir][w * y + x] = ipy;
                            jumpd[dir][w * y + x] = ipd;
                            jumpv[dir][w * y + x] = value;
                        }
                        done = true;
                    }
                }
            }
        }
    }

    //begin the interpretation
    #define MEMORYSIZE 1024 //the memory allocation will increase by this size everytime it's full

    std::vector<char> a;
    a.resize(MEMORYSIZE);
    int p = 0; //pointer in the array a
    int ipx = 0, ipy = 0, ipd = 0; //instruction pointer location and direction
    
    int command;
    
    bool done = false;
    while(!done)
    {
        command = code[w * ipy + ipx];
        switch(command)
        {
            case '>':
                if(jumpv[ipd][w * ipy + ipx] < 0) p++;
                else
                {
                    p += jumpv[ipd][w * ipy + ipx];
                    int dir = ipd, index = w * ipy + ipx;
                    ipx = jumpx[dir][index];
                    ipy = jumpy[dir][index];
                    ipd = jumpd[dir][index];
                }
                if(p >= a.size()) a.resize(p + MEMORYSIZE, 0);
                break;
            case '<': //darkred <
                if(jumpv[ipd][w * ipy + ipx] < 0) p--;
                else
                {
                    p -= jumpv[ipd][w * ipy + ipx];
                    int dir = ipd, index = w * ipy + ipx;
                    ipx = jumpx[dir][index];
                    ipy = jumpy[dir][index];
                    ipd = jumpd[dir][index];
                }
                if(p < 0) p = 0;
                break;
            case '+': //green +
                if(jumpv[ipd][w * ipy + ipx] < 0) a[p]++;
                else
                {
                    a[p] += jumpv[ipd][w * ipy + ipx];
                    int dir = ipd, index = w * ipy + ipx;
                    ipx = jumpx[dir][index];
                    ipy = jumpy[dir][index];
                    ipd = jumpd[dir][index];
                }
                break;
            case '-': //darkgreen -
                if(jumpv[ipd][w * ipy + ipx] < 0) a[p]--;
                else
                {
                    a[p] -= jumpv[ipd][w * ipy + ipx];
                    int dir = ipd, index = w * ipy + ipx;
                    ipx = jumpx[dir][index];
                    ipy = jumpy[dir][index];
                    ipd = jumpd[dir][index];
                }
                break;
            case '.': //blue .
                cout << a[p];
                break;
            case ',': //darkblue ,
                a[p] = getchar();
                break;
            case '[': //yellow [
                if(a[p] == 0)
                {
                    if(jumpd[ipd][w * ipy + ipx] < 0)
                    {
                        int tofind = 1;
                        while(tofind > 0)
                        {
                            moveIP(ipx, ipy, ipd);
                            if(ipx < 0 || ipx >= w || ipy < 0 || ipy >= h) {done = true; tofind = 0; continue;}
                            command = code[w * ipy + ipx];
                            switch(command)
                            {
                                case '[': tofind++; break;
                                case ']': tofind--; break;
                                case 'r': rotright(ipd); break;
                                case 'l': rotleft(ipd); break;
                                default: break;
                            }
                        }
                    }
                    else
                    {
                        int dir = ipd, index = w * ipy + ipx;
                        ipx = jumpx[dir][index];
                        ipy = jumpy[dir][index];
                        ipd = jumpd[dir][index];
                    }
                }
                break;
            case ']': //dark yellow ]
                if(a[p] != 0)
                {
                    if(jumpd[ipd][w * ipy + ipx] < 0)
                    {
                        int tofind = 1;
                        while(tofind > 0)
                        {
                            moveIPBack(ipx, ipy, ipd);
                            if(ipx < 0 || ipx >= w || ipy < 0 || ipy >= h) {done = true; tofind = 0; continue;}
                            command = code[w * ipy + ipx];
                            switch(command)
                            {
                                case '[': tofind--; break;
                                case ']': tofind++; break;
                                case 'r': rotleft(ipd); break;
                                case 'l': rotright(ipd); break;
                                default: break;
                            }
                        }
                    }
                    else
                    {
                        int dir = ipd, index = w * ipy + ipx;
                        ipx = jumpx[dir][index];
                        ipy = jumpy[dir][index];
                        ipd = jumpd[dir][index];
                    }
                }
                break;
            case 'r': //cyan
                rotright(ipd);
                break;
            case 'l': //darkcyan
                rotleft(ipd);
                break;
            default: //NOP
                if(jumpd[ipd][w * ipy + ipx] >= 0) //jump over a series of nops
                {
                    int dir = ipd, index = w * ipy + ipx;
                    ipx = jumpx[dir][index];
                    ipy = jumpy[dir][index];
                    ipd = jumpd[dir][index];
                }
                break;
        }
        
        moveIP(ipx, ipy, ipd);
        if(ipx < 0 || ipx >= w || ipy < 0 || ipy >= h) done = true;
    }



    cout << "\n";
    return 0;
}
/*
Braincopter is a brainloller clone. The specification is the same as that of brainloller, except the color values for the commands. In braincopter every color is translated into one of the 11 commands, making it possible to truly use a photo as code.

When using the pngtobmp converter provided with lollercopter, the brainfuck code in the photo is almost invisible to the eye.

Translation of braincopter commands into brainfuck:

command = (65536 * R + 256 * G + B) % 11, where % is a modulo division and R, G, B are the color components of the pixel.

command    function
----------------------------------------
0          >
1          <
2          +
3          -
4          .
5          ,
6          [
7          ]
8          rotate IP to the right
9          rotate IP to the left
10         NOP

The rest of the specification is exactly the same as that of brainloller, so it's not duplicated here.
*/

