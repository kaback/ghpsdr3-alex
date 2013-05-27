/* Copyright (C) - modifications of the original program by John Melton
* 2012 - Alex Lee, 9V1Al
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Pl
*/

#include "Waterfallgl.h"

Waterfallgl::Waterfallgl()
    : ShaderProgram(0)
{
}

Waterfallgl::~Waterfallgl(){
}

void Waterfallgl::initializeGL(){
    initializeOpenGLFunctions();
}

static char const vertexShader[] =
        "attribute vec4 aPosition;\n"
        "attribute vec2 aTextureCoord;\n"
        "uniform mat4 uMVPMatrix;\n"
        "varying vec2 vTextureCoord;\n"
        "void main()\n"
        "{\n"
        "vTextureCoord = aTextureCoord;\n"
        "gl_Position = uMVPMatrix * aPosition;\n"
        "}\n";

static char const fragmentShader[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D spectrumTexture;\n"
        "uniform float cy;\n"
        "uniform float offset;\n"
        "uniform float width;\n"
        "uniform float waterfallLow, waterfallHigh;\n"
        "varying vec2 vTextureCoord;\n"
        "void main()\n"
        "{\n"
            "float y_coord = vTextureCoord.t + cy;\n"
            "if (y_coord > 1.0) y_coord -= 1.0;\n"
            "float x_coord = vTextureCoord.s - offset;\n"
            "if (x_coord < 0.0) x_coord += width;\n"
            "if (x_coord > width) x_coord -= width;\n"
            "vec4 value = texture2D(spectrumTexture, vec2(x_coord, y_coord));\n"
            "float sample = 0.0 - value.r;\n"
            "float percent = (sample - waterfallLow)/(waterfallHigh-waterfallLow);\n"
            "if (percent < 0.0) percent = 0.0;\n"
            "if (percent > 1.0) percent = 1.0;\n"
            "vec4 texel;\n"
            "if (percent < (2.0/9.0)) {texel = vec4(0.0, 0.0, percent/(2.0/9.0), 1.0);}\n"
                "else if (percent < (3.0/9.0)) {texel = vec4(0.0, (percent - (2.0/9.0))/(1.0/9.0), 1.0, 1.0);}\n"
                "else if (percent < (4.0/9.0)) {\n"
                "float local_percent = (percent - (3.0/9.0))/(1.0/9.0);\n"
                "texel = vec4(0.0, (1.0 - local_percent), 1.0, 1.0);\n"
                 "}\n"
            "else if (percent < (5.0/9.0)) {texel = vec4((percent - (4.0/9.0))/(1.0/9.0), 1.0, 0.0, 1.0);}\n"
            "else if (percent < (7.0/9.0)) {\n"
                "float local_percent = (percent - (5.0/9.0))/(2.0/9.0);\n"
                "texel = vec4(1.0, (1.0 - local_percent), 0.0, 1.0);\n"
                "}\n"
            "else if (percent < (8.0/9.0)){ texel = vec4(1.0, 0.0, (percent - (7.0/9.0))/(1.0/9.0), 1.0);}\n"
            "else {\n"
                "float local_percent = (percent - 8.0/9.0)/(1.0/9.0);\n"
                "texel = vec4(0.75+0.25*(1.0-local_percent), 0.5*local_percent, 1.0, 1.0);\n"
                "}\n"
            "gl_FragColor = texel;\n"
        "}\n";

void Waterfallgl::initialize(int wid, int ht){

    data_width = wid;
    data_height = ht;
    cy = MAX_CL_HEIGHT - 1;

    ShaderProgram = new QOpenGLShaderProgram(this);
    ShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
    ShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
    ShaderProgram->link();

    spectrumTexture_location = ShaderProgram->uniformLocation("spectrumTexture");
    cy_location =  ShaderProgram->uniformLocation("cy");
    offset_location =  ShaderProgram->uniformLocation("offset");
    waterfallLow_location =  ShaderProgram->uniformLocation("waterfallLow");
    waterfallHigh_location =  ShaderProgram->uniformLocation("waterfallHigh");
    width_location =  ShaderProgram->uniformLocation("width");
    aPosition_location = ShaderProgram->attributeLocation("aPosition");
    textureCoord_location = ShaderProgram->attributeLocation("aTextureCoord");
    uMVPMatrix_location = ShaderProgram->uniformLocation("uMVPMatrix");

    static unsigned char data[MAX_CL_WIDTH][MAX_CL_HEIGHT];
    for (int i = 0; i < MAX_CL_HEIGHT; i++){
        for (int j = 0; j < MAX_CL_WIDTH; j++){
            data[i][j]= (unsigned char) 0xff;
        }
    }
    glGenTextures(1, &spectrumTex);
    glBindTexture(GL_TEXTURE_2D, spectrumTex);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, MAX_CL_WIDTH, MAX_CL_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLvoid*) data);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

#ifdef GL_CLAMP_TO_EDGE
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif

}

void Waterfallgl::setHigh(int high) {
    float wfHigh = (float) (waterfallHigh) / 256.0f;
    glUniform1f(waterfallHigh_location, wfHigh);
    waterfallHigh=high;
}

void Waterfallgl::setLow(int low) {
    waterfallLow=low;
    float wfLow = (float) (waterfallLow) / 256.0f;
    glUniform1f(waterfallLow_location, wfLow);
}

void Waterfallgl::setAutomatic(bool state) {
    waterfallAutomatic=state;
}

void Waterfallgl::setLO_offset(GLfloat offset){
    LO_offset = offset;
    glUniform1f(offset_location, LO_offset);
}


void Waterfallgl::resizeGL( int width, int height )
{
    data_width = width;
    data_height = height;

    height = height?height:1;
    glViewport( 0, 0, (GLint)width, (GLint)height );
}

void Waterfallgl::paintGL()
{

    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT);

    //Bind to tex unit 0
    glUniform1i(spectrumTexture_location, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spectrumTex);
    float current_line = (float) cy /  MAX_CL_HEIGHT;
    glUniform1f(cy_location, current_line);
    GLfloat tex_width = (float) data_width / MAX_CL_WIDTH;

    glUniform1f(width_location, tex_width);

    // Ortho2D projection
    const GLfloat mMVPMatrix[] = {2.0f/data_width, 0.0f, 0.0f, 0.0f,
                           0.0f, -2.0f/data_height, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           -1.0f, 1.0f, 0.0f, 1.0f
                           };

    glUniformMatrix4fv(uMVPMatrix_location, 1, false, mMVPMatrix);

    const GLfloat mVertices[] =  {
        0.0f, 0.0f,  // Position 0
        0.0f, 0.0f,  // TexCoord 0
        (float)data_width, 0.0f,  // Position 1
        tex_width, 0.0f, // TexCoord 1
        (float)data_width, MAX_CL_HEIGHT, // Position 2
        tex_width, 1.0f, // TexCoord 2
        0.0f, MAX_CL_HEIGHT, // Position 3
        0.0f, 1.0f // TexCoord 3
    };
    glVertexAttribPointer(aPosition_location, 2, GL_FLOAT, false, sizeof(GLfloat)*4, mVertices);
    glEnableVertexAttribArray(aPosition_location);
    glVertexAttribPointer(textureCoord_location, 2, GL_FLOAT, false, sizeof(GLfloat)*4, &mVertices[2]);
    glEnableVertexAttribArray(textureCoord_location);

    const GLshort mIndices[] =
    {
        0, 1, 2, 0, 2, 3
    };

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices );

}

void Waterfallgl::updateWaterfall(char *buffer, int width, int starty){
    setLO_offset(0.0);

    int sum = 0;
    for(int i=0;i<width;i++) sum += -(buffer[i] & 0xFF);
    average = average * 0.99f + (float)sum/(float)width * 0.01f; // running avera
    setLow(average - 10);
    setHigh(average + 50);

    int data_length = (width < MAX_CL_WIDTH) ? width : MAX_CL_WIDTH;
    if (cy-- <= 0) cy = MAX_CL_HEIGHT - 1;

    unsigned char data[MAX_CL_WIDTH];
    for (int i = 0; i < data_length; i++){
        data[i] = buffer[i];
    }

    // Update Texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, cy, MAX_CL_WIDTH, 1,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLvoid*)data);
}


