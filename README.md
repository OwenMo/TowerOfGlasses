Source code distribution Under http://creativecommons.org/licenses/by-nc/3.0/

Name: Jenis Modi

Date: Dec 7 2012

Project Authors:

Jenis Modi

Dr. Wayne Cochran



Texture images: Downloaded from google images.

The project creates Glass of tower as defined in Project document. It contains 3 "C" files, 1 vertex shader file and 1 fragment shader file. Initial level of glasses is defined as 2. 

1. wineGlassTexture.c : Generate tower of glasses on table.
2. glass.c: Generate glass related vertices and indices
3. matrix.c: used for extra matrix related operation

Additional feature: 

	Press '1': To generate 1 level of glasses ( 1 glass)
  
  Press '2': To generate 2 level of glasses ( 3 glasses)
  
  Press '3': To generate 3 level of glasses ( 6 glasses)
  
  Press '4': To generate 4 level of glasses ( 10 glasses)
  
  Press '5': To generate 5 level of glasses ( 15 glasses)
  
  OR
  
  Use mouse Right click menu to generate above number of glasses.
  
  Press 'z' or 'x' : Zoom in or out
  
  Mouse scrolling: Zoom in/out (Did not work in Lab Mac computers)
  
  Press 'Esc': To Exit


The project is coded in C Language. I have a make file to execute my code. You can just use “make” command to compile the code and then ./wineGlassTexture to run the code. The code is compiled with gnu99 option in the makefile. If this is not supported in user's machine, please change it to c99.

