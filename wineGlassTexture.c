#define GL_GLEXT_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "matrix.h"
#include "glass.h"
#if defined(__APPLE__) || defined(MACOSX)
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define RADIANS_PER_PIXEL M_PI/(2*90.0)

/**
Authors: Jenis Modi
         Wayne Cochran

Desc: Generate a Glass Tower with texture, reflection and shadow

Keys: 
  Press '1': To generate 1 level of glasses ( 1 glass)
	Press '2': To generate 2 level of glasses ( 3 glasses)
	Press '3': To generate 3 level of glasses ( 6 glasses)
	Press '4': To generate 4 level of glasses ( 10 glasses)
	Press '5': To generate 5 level of glasses ( 15 glasses)
		OR
	Use mouse Right click menu to generate above number of glasses.

	Press z/x: Zoom in/out

	Mouse scrolling: Zoom in/out (Did not work in Lab Mac computers)

	Press 'Esc': To Exit
	
	Use mouse left button to move the image

--Help from Ryan for light reflection in the vertex shader


**/

  typedef struct {
    int W, H;            /* image size */
    GLubyte (*rgba)[4];  /* WxH row-major order buffer of RGBA values */
  } Image;

  static GLuint texName[2]; 

  GLfloat Color[4];
  double centerX=0,centerY=0,centerZ=0;
  double upX=0,upY=0,upZ=1;

  GLfloat center;

  double eyeX,eyeY,eyeZ;

  int mouseX,mouseY;
  int N_FACTOR;
  double phi = 0.587266,radius=5,theta=0.285398;

  GLboolean changeFlag;


  void setCamera();


  GLfloat ModelView[4*4];
  GLfloat Projection[4*4];


  GLfloat ambientLight[3] = {0.2, 0.2, 0.2};
  GLfloat light0Color[3] = {1.0, 1.0, 1.0};
  GLfloat light0Position[3] = {10.0, 10.0, 10.0};


  GLfloat materialAmbient[3];
  GLfloat materialDiffuse[3];

  GLfloat materialSpecular[3];
  GLfloat materialShininess;

  GLint vertexPositionAttr;
  GLint vertexNormalAttr;
  GLint vertexTexCoordAttr;

  GLint ModelViewProjectionUniform;
  GLint ModelViewMatrixUniform;
  GLint NormalMatrixUniform;
  GLint TextureMatrixUniform;
  GLint refModelViewMatrixUniform;

  GLint ambientLightUniform;
  GLint light0ColorUniform;
  GLint light0PositionUniform;

  GLint materialAmbientUniform;
  GLint materialDiffuseUniform;
  GLint materialSpecularUniform;
  GLint materialShininessUniform;

/**
* conditional variables used for fragment
**/

  GLint nonShadowCodeUniform;
 
  GLint TexUnitUniform;
  GLint ColorUniform;

  GLuint vertexShader;
  GLuint fragmentShader;
  GLuint program;

  void checkOpenGLError(int line) {
    bool wasError = false;
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        printf("GL ERROR: at line %d: %s\n", line, gluErrorString(error));
        wasError = true;
        error = glGetError();
    }
    if (wasError) exit(-1);
  }

/*
 * Get raw PPM (Portable Pixel Map) image from file.
 * Image buffer padded so its width and height are powers of 2.
 * Each "pixel" is 4 bytes wide and holds RGBA components (all alpha
 * components are set to 255).
 */
  static void getppm(char *fname, Image *image) {
    FILE *f;
    static char buf[100];
    GLboolean gotSize, gotMax;
    int r, c;
    int W,H;
    GLubyte (*rgba)[4];
  
    if ((f = fopen(fname, "rb")) == NULL) {
      perror(fname);
      exit(-1);
    }
  
    if (fgets(buf, sizeof(buf), f) == NULL || strncmp("P6", buf, 2) != 0) {
      fprintf(stderr, "Bogus magic string for raw PPM file '%s'!\n", fname);
      exit(-1);
    }
  
    for (gotSize = gotMax = GL_FALSE; !gotMax; ) {
      if (fgets(buf, sizeof(buf), f) == NULL) {
        fprintf(stderr, "Error parsing header for PPM file '%s'!\n", fname);
        exit(-1);
      }
      if (buf[0] == '#')
        continue;
      if (!gotSize) {
        if (sscanf(buf, "%d %d", &W, &H) != 2 || W <= 0 || H <= 0) {
          fprintf(stderr, "Bogus size for for PPM file '%s'!\n", fname);
          exit(-1);
        }
        gotSize = GL_TRUE;
      } else {
        int max;
        if (sscanf(buf, "%d", &max) != 1 || max < 2 || max > 255) {
          fprintf(stderr, "Bogus max intensity for PPM file '%s'!\n", fname);
          exit(-1);
        }
        gotMax = GL_TRUE;
      }
    }
    
    if ((rgba = (GLubyte (*)[4]) calloc(W*H,4)) == NULL) {
      perror("calloc() image");
      exit(-1);
    }

    for (r = H-1; r >= 0; r--)    /* flip y around! */
      for (c = 0; c < W; c++) {
        unsigned char rgb[3];
        int index;
        if (fread(rgb, 1, 3, f) != 3) {
          if (ferror(f))
            perror(fname);
          else
            fprintf(stderr, "Unexpected EOF: %s\n", fname);
          exit(-1);
        }
        index = r*W + c;
        rgba[index][0] = rgb[0];
        rgba[index][1] = rgb[1];
        rgba[index][2] = rgb[2];
        rgba[index][3] = 255;
      }

    fclose(f);

    image->W = W;
    image->H = H;
    image->rgba = rgba;
  }

  static void getTexture(void){
  char *ppmName = "table.ppm";

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glGenTextures(2, texName);
  Image image;

  getppm(ppmName, &image);
  glBindTexture(GL_TEXTURE_2D, texName[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.W, image.H, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba);
  free(image.rgba);
	
  ppmName = "glass.ppm";
  getppm(ppmName, &image);
  glBindTexture(GL_TEXTURE_2D, texName[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.W, image.H, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba);
  free(image.rgba);
  }


  void loadUniforms() {
    GLfloat ModelViewProjection[4*4], NormalMatrix[3*3];
    GLfloat TextureMatrix[4*4];
    matrixMultiply(ModelViewProjection, Projection, ModelView);
    matrixNormal(ModelView, NormalMatrix);
    matrixIdentity(TextureMatrix);
    glUniformMatrix4fv(ModelViewProjectionUniform, 1, GL_FALSE,
                       ModelViewProjection);
    glUniformMatrix4fv(ModelViewMatrixUniform, 1, GL_FALSE,
                       ModelView);
    glUniformMatrix3fv(NormalMatrixUniform, 1, GL_FALSE, NormalMatrix);
    glUniformMatrix4fv(TextureMatrixUniform, 1, GL_FALSE, TextureMatrix);

    //
    // Load lights.
    //
    glUniform3fv(ambientLightUniform, 1, ambientLight);
    glUniform3fv(light0ColorUniform, 1, light0Color);
    glUniform3fv(light0PositionUniform, 1, light0Position);
    

    //
    // Load material properties.
    //
    glUniform3fv(materialAmbientUniform, 1, materialAmbient);
    glUniform3fv(materialDiffuseUniform, 1, materialDiffuse);
    glUniform3fv(materialSpecularUniform, 1, materialSpecular);
    glUniform1f(materialShininessUniform, materialShininess);

    //
    // Only using texture unit 0.
    //
    glUniform3fv(ColorUniform, 1, Color);
    glUniform1i(TexUnitUniform, 0);


  }

  GLchar *getShaderSource(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (f == NULL) {
        perror(fname); exit(-1);
    }
    fseek(f, 0L, SEEK_END);
    int len = ftell(f);
    rewind(f);
    GLchar *source = (GLchar *) malloc(len + 1);
    if (fread(source,1,len,f) != len) {
        if (ferror(f))
            perror(fname);
        else if (feof(f))
            fprintf(stderr, "Unexpected EOF when reading '%s'!\n", fname);
        else
            fprintf(stderr, "Unable to load '%s'!\n", fname);
        exit(-1);
    }
    source[len] = '\0';
    fclose(f);
    return source;
  }

//
// Install our shader programs and tell GL to use them.
// We also initialize the uniform variables.
//
  void installShaders(void) {
    //
    // (1) Create shader objects
    //
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    //
    // (2) Load source code into shader objects.
    //
    const GLchar *vertexShaderSource = getShaderSource("vertex.vs");
    const GLchar *fragmentShaderSource = getShaderSource("fragment-tex.fs");

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);

    //
    // (3) Compile shaders.
    //
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "vertex shader info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "fragment shader info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    //
    // (4) Create program object and attach vertex and fragment shader.
    //
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    checkOpenGLError(__LINE__);

    //
    // (5) Link program.
    //
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[200];
        GLint charsWritten;
        glGetProgramInfoLog(program, sizeof(infoLog), &charsWritten, infoLog);
        fprintf(stderr, "program info log:\n%s\n\n", infoLog);
    }
    checkOpenGLError(__LINE__);

    //
    // (7) Get vertex attribute locations
    //
    vertexPositionAttr = glGetAttribLocation(program, "vertexPosition");
    vertexNormalAttr = glGetAttribLocation(program, "vertexNormal");
    vertexTexCoordAttr = glGetAttribLocation(program, "vertexTexCoord");

    if (vertexPositionAttr == -1 || vertexNormalAttr == -1 || vertexTexCoordAttr == -1) {
        fprintf(stderr, "Error fetching vertex position or normal attribute or texture Attribute!\n");

    }

    //
    // (8) Fetch handles for uniform variables in program.
    //
    ModelViewProjectionUniform = glGetUniformLocation(program, "ModelViewProjection");
    if (ModelViewProjectionUniform == -1) {
        fprintf(stderr, "Error fetching modelViewProjectionUniform		!\n");
        exit(-1);
    }

    ModelViewMatrixUniform = glGetUniformLocation(program, "ModelViewMatrix");

    if (ModelViewMatrixUniform == -1) {
        fprintf(stderr, "Error fetching modelViewMatrixUniform!\n");
        // exit(-1);
    }

    NormalMatrixUniform = glGetUniformLocation(program, "NormalMatrix");

    if (NormalMatrixUniform == -1) {
        fprintf(stderr, "Error fetching normalMatrixUniform!\n");
        // exit(-1);
    }

    TextureMatrixUniform = glGetUniformLocation(program, "TextureMatrix");

    if (TextureMatrixUniform == -1) {
        fprintf(stderr, "Error fetching TextureMatrixUniform!\n");
    }

    ambientLightUniform = glGetUniformLocation(program, "ambientLight");

    if (ambientLightUniform == -1) {
        fprintf(stderr, "Error fetching ambientLightUniform!\n");
        exit(-1);
    }

    light0ColorUniform = glGetUniformLocation(program, "light0Color");

    if (light0ColorUniform == -1) {
        fprintf(stderr, "Error fetching light0ColorUniform!\n");
        // exit(-1);
    }

    light0PositionUniform = glGetUniformLocation(program, "light0Position");

    if (light0PositionUniform == -1) {
        fprintf(stderr, "Error fetching light0PositionUniform!\n");
        // exit(-1);
    }

    materialAmbientUniform = glGetUniformLocation(program, "materialAmbient");

    if (materialAmbientUniform == -1) {
        fprintf(stderr, "Error fetching materialAmbientUniform!\n");
        // exit(-1);
    }

    materialDiffuseUniform = glGetUniformLocation(program, "materialDiffuse");

    if (materialDiffuseUniform == -1) {
        fprintf(stderr, "Error fetching materialDiffuseUniform!\n");
        //exit(-1);
    }

    materialSpecularUniform = glGetUniformLocation(program, "materialSpecular");


    materialShininessUniform = glGetUniformLocation(program, "materialShininess");
    TexUnitUniform = glGetUniformLocation(program, "texUnit");
    nonShadowCodeUniform = glGetUniformLocation(program, "nonShadowCodeUniform");
  //  lightReflectUniform = glGetUniformLocation(program, "lightReflectUniform");
    refModelViewMatrixUniform = glGetUniformLocation(program, "refModelViewMatrix");
    //
    // (9) Tell GL to use our program
    //
    glUseProgram(program);
  }

  void glass(void) {
    static GLuint vertBuffer;
    static GLuint normalBuffer;
    static GLuint indexBuffer;
    static GLuint glassCoordsBuffer;


    static GLboolean first = GL_TRUE;
    if (first) {

    
      glGenBuffers(1, &vertBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
      glBufferData(GL_ARRAY_BUFFER, NUM_GLASS_VERTS*3*sizeof(GLfloat),
                 glassVerts, GL_STATIC_DRAW);

      glGenBuffers(1, &normalBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
      glBufferData(GL_ARRAY_BUFFER, NUM_GLASS_VERTS*3*sizeof(GLfloat),
                 glassNormals, GL_STATIC_DRAW);

      glGenBuffers(1, &indexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                 NUM_GLASS_QUAD_STRIP_INDICES*sizeof(GLushort),
                 glassQuadStripIndices, GL_STATIC_DRAW);

      glGenBuffers(1, &glassCoordsBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, glassCoordsBuffer);
      glBufferData(GL_ARRAY_BUFFER, NUM_GLASS_VERTS*2*sizeof(GLfloat),glassTexCoords, GL_STATIC_DRAW);

    
      first = GL_FALSE;
    }

    loadUniforms();

  

      glEnableVertexAttribArray(vertexPositionAttr);
      glBindBuffer(GL_ARRAY_BUFFER, vertBuffer); 
      glVertexAttribPointer(vertexPositionAttr, 3, GL_FLOAT, 
                        GL_FALSE, 0, (GLvoid*) 0);

      glEnableVertexAttribArray(vertexNormalAttr);
      glBindBuffer(GL_ARRAY_BUFFER, normalBuffer); 
      glVertexAttribPointer(vertexNormalAttr, 3, GL_FLOAT, 
                        GL_FALSE, 0, (GLvoid*) 0);
  
      glEnableVertexAttribArray(vertexTexCoordAttr);
      glBindBuffer(GL_ARRAY_BUFFER, glassCoordsBuffer);
      glVertexAttribPointer(vertexTexCoordAttr, 2, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid*) 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer); 

      for (int strip = 0; strip < NUM_GLASS_QUAD_STRIPS; strip++) {
	const int offset = VERTS_PER_GLASS_QUAD_STRIP*sizeof(GLushort)*strip;
	glDrawElements(GL_QUAD_STRIP, VERTS_PER_GLASS_QUAD_STRIP,
                   GL_UNSIGNED_SHORT, (GLchar*) 0 + offset);
      }
  
  }

  void bunchOfglasses(void) {

    for (int i = 0; i < 3; i++) {
    	materialAmbient[i] = 0.1;
    	materialSpecular[i] = 0.3;
    }
    materialDiffuse[0] = 1.0;
    materialDiffuse[1] = 1.0;
    materialDiffuse[2] = 1.0; 
    materialShininess = 200.0;
	
    float scaleFactor = (float) 1/N_FACTOR;
    GLfloat x,y,z;
    GLfloat xArr[10];
    z = 0.0;
    int i,j;
    y = -0.5;
    x = -scaleFactor;
    glBindTexture(GL_TEXTURE_2D, texName[1]);
    
    for(i=1; i<=N_FACTOR; i++){
	for(j=N_FACTOR; j>=i; j--){
          xArr[j] = x;
	  matrixPush(ModelView);
	  matrixTranslate(ModelView, x, y, z);
	  glass();
     	  matrixPop(ModelView);		
	  x-= 0.30;		
	}
	x =  (xArr[N_FACTOR] + xArr[N_FACTOR-1])/2.0;
	z+=0.50;
    }
    matrixScale(ModelView,scaleFactor,scaleFactor,scaleFactor);
  }

  void plane(void) {
    static GLuint vertBuffer;
    static GLuint normalBuffer;
    static GLuint indexBuffer;
    static GLuint texCoordBuffer;
    const GLfloat samples = 50;
    static GLboolean first = GL_TRUE;
  
    if (first) {
    //
    // Allocate buffer 'verts' to hold vertex positions
    // and (later) surface normals.
    //

      const GLfloat radius = 3.0;
      const size_t vertBufferSize = samples*samples*3*sizeof(GLfloat);
      GLfloat *verts = (GLfloat*) malloc(vertBufferSize);

    //
    // Compute grid vertex positions and load into VBO.
    //
      const GLfloat ds = 2*radius/(samples - 1);
      GLfloat *vp = verts;
      float y = radius;

      for (int j = 0; j < samples; j++, y -= ds) {
        float x = -radius;
          for (int i = 0; i < samples; i++, x += ds) {
            *(vp++) = x;
            *(vp++) = y;
            *(vp++) = 0;
          }
      }
      glGenBuffers(1, &vertBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
      glBufferData(GL_ARRAY_BUFFER, vertBufferSize, verts, GL_STATIC_DRAW);

    //
    // Assign surface normals (all [0,0,1]) and load into VBO.
    //
      vp = verts;
      for (int j = 0; j < samples; j++) {
        for (int i = 0; i < samples; i++) {
          *(vp++) = 0;
          *(vp++) = 0;
          *(vp++) = 1;
        }
      }
    
      glGenBuffers(1, &normalBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
      glBufferData(GL_ARRAY_BUFFER, vertBufferSize, verts, GL_STATIC_DRAW);



      verts = realloc(verts, sizeof(GLfloat) * samples * samples * 2);
      vp = verts;

      for(int j = 0; j < samples; j++){
    	for(int i = 0; i < samples; i++){
    	  *(vp++) = i/samples;
    	  *(vp++) = j/samples;
    	}
      }

      glGenBuffers(1, &texCoordBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * samples * samples * 2, verts, GL_DYNAMIC_DRAW);

      free(verts);
    //
    // Compute and load mesh indices into VBO.
    //
      const size_t indicesBufferSize = 2*samples*(samples-1)*sizeof(GLushort);
      GLushort *indices = (GLushort*) malloc(indicesBufferSize);
      GLushort *ip = indices;
      for (int j = 0; j < samples-1; j++){
        for (int i = 0; i < samples; i++) {
	  *(ip++) = (j+1)*samples + i;
          *(ip++) = j*samples + i;        
        }
      }
        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBufferSize,
                 indices, GL_STATIC_DRAW);
        free(indices);

	
        first = GL_FALSE;  // never do this again.
      }

      loadUniforms();
      glEnableVertexAttribArray(vertexPositionAttr);
      glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
      glVertexAttribPointer(vertexPositionAttr, 3, GL_FLOAT,
                        GL_FALSE, 0, (GLvoid*) 0);

      glEnableVertexAttribArray(vertexNormalAttr);
      glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
      glVertexAttribPointer(vertexNormalAttr, 3, GL_FLOAT,
                        GL_FALSE, 0, (GLvoid*) 0);
  
      glEnableVertexAttribArray(vertexTexCoordAttr);
      glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);  
      glVertexAttribPointer(vertexTexCoordAttr, 2, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid*) 0);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);


      for (int j = 0; j < samples-1; j++) {
     	const int offset = 2*samples*sizeof(GLushort)*j;
	glDrawElements(GL_QUAD_STRIP, 2*samples,
                   GL_UNSIGNED_SHORT, (GLchar*) 0 + offset);
      }  
 
  }

  void getPlane(void){
	for(int i = 0; i < 3; i++){
		materialSpecular[i] = 1.0;
		materialAmbient[i] = 0.5;
		materialDiffuse[i] = 1.0;
	}
	materialShininess = 200.0;
	glBindTexture(GL_TEXTURE_2D, texName[0]);
	plane();
  }

  void sphericalToCartesian(double radius, double theta, double phi,
                          double *x_Cart, double *y_Cart, double *z_Cart) {
    *x_Cart = radius*cos(theta)*sin(phi);
    *y_Cart = radius*sin(theta)*sin(phi);
    *z_Cart = radius*cos(phi);
  }

  void display(void) {
    static GLfloat tablePlane[4] = {0, 0, 1, 0};
    glUniformMatrix4fv(refModelViewMatrixUniform, 1, GL_FALSE, ModelView);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Color);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    
    glUniform1i(nonShadowCodeUniform,1);
   
/**
** Draw glass tower
**/
    matrixPush(ModelView);
    bunchOfglasses();
    matrixPop(ModelView);
	
/**
** Draw Table
**/
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,1,1);
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    getPlane();

/**
** Draw Reflection
**/

    glUniform1i(nonShadowCodeUniform,1);

    glClear(GL_DEPTH_BUFFER_BIT);
    glStencilFunc(GL_EQUAL,1, 1);
    glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
    matrixPush(ModelView);
    matrixScale(ModelView,1,1,-1);
    bunchOfglasses();
    matrixPop(ModelView);

/**
** Table Alpha blend
**/

    glEnable(GL_BLEND);
    glBlendColor(1,1,1,0.5);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    glCullFace(GL_FRONT);
    getPlane();

/**
** Draw Shadow
**/ 
    glUniform1i(nonShadowCodeUniform,0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    matrixPush(ModelView);
    matrixShadow(ModelView, light0Position, tablePlane);
    glPolygonOffset(-1.0,-1.0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    bunchOfglasses();
    glDisable(GL_POLYGON_OFFSET_FILL);
    matrixPop(ModelView);
  
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glUniform1i(nonShadowCodeUniform,1);

    glutSwapBuffers();
  }

  void keyboard(unsigned char key, int x, int y) {
  #define ESC 27

    if (key == ESC) 
      exit(0);
    else if(key == 'z' && radius < 8 ) {
      radius-=0.1;
      setCamera();
      glutPostRedisplay();
    }else if(key == 'x' && radius > 0.3) {
      radius+=0.1;
      setCamera();
      glutPostRedisplay();
    }else if(key == '1'){
      N_FACTOR = 1; 
    }else if(key == '2'){
      N_FACTOR = 2;
    }else if(key == '3'){
      N_FACTOR = 3;
    }else if(key == '4'){
      N_FACTOR = 4;
    }else if(key == '5'){
      N_FACTOR = 5;
    }
  }

  void mouse(int button, int state, int x, int y) {

    mouseX = x;
    mouseY = y;
    if(button == 3 && radius >0.3){
	radius-=0.1;
	setCamera();
	glutPostRedisplay();
    } else if(button == 4 && radius < 8){
	radius += 0.1;
	setCamera();
	glutPostRedisplay();
    }
	
	
  }

  #define RADIANS_PER_PIXEL M_PI/(2*90.0)
  #define EPSILON 0.00001
  void mouseMotion(int x, int y) {
    float mouseBound = 0.5;
    int dx = x - mouseX;
    int dy = y - mouseY;
    theta -= dx * RADIANS_PER_PIXEL;
    phi -= dy * RADIANS_PER_PIXEL;
    if (phi < mouseBound)
        phi = mouseBound;
    else if (phi > M_PI/2 - mouseBound)
        phi = M_PI/2 - mouseBound;

    setCamera();
    mouseX = x;
    mouseY = y;
    glutPostRedisplay();
  }


  void idle(){
    GLfloat seconds = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    GLfloat rotateSpeed = 360.0/40;
    center = rotateSpeed*seconds;
    glutPostRedisplay();

  } 

  void initValues(){
    N_FACTOR = 2;
    changeFlag = GL_FALSE;
  }


  void setCamera() {
    sphericalToCartesian(radius, theta, phi,
                         &eyeX, &eyeY, &eyeZ);
    eyeX += centerX; eyeY += centerY; eyeZ += centerZ;

    matrixIdentity(ModelView);
    matrixLookat(ModelView, eyeX,    eyeY,    eyeZ,
                 centerX, centerY, centerZ,
                 upX,     upY,     upZ);
  }

  static void displayMenu(int value){

    switch (value) {
      case 1:
        N_FACTOR = 1;
        break;
      case 2:
        N_FACTOR = 2;
        break;
      case 3:
        N_FACTOR = 3;
        break;
      case 4:
        N_FACTOR = 4;
        break;
      case 5:
	N_FACTOR = 5;
	break;
      case 6:
        exit(0);
        break;
    }

  }

  int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800,800);
    glutInitWindowPosition(10,10);
    
    initValues();
    
    glutCreateWindow("Texture Reflection Shadow");
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    glutCreateMenu(displayMenu);
    glutAddMenuEntry("N = 1",1);
    glutAddMenuEntry("N = 2",2);
    glutAddMenuEntry("N = 3",3);
    glutAddMenuEntry("N = 4",4);
    glutAddMenuEntry("N = 5",5);
    glutAddMenuEntry("Exit",6);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    installShaders();
    getTexture();
    setCamera();

    matrixIdentity(Projection);
    matrixPerspective(Projection,
                      40, 1.0, (GLfloat) 0.3, 250);

    glClearColor(1.0,1.0,1.0,1.0);

    glutMainLoop();
    return 0;
  }
