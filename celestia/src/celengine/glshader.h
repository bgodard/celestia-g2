// glshader.h
//
// Copyright (C) 2001-2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_GLSHADER_H_
#define _CELENGINE_GLSHADER_H_

#include <string>
#include <vector>


class GLShaderLoader;

enum GLShaderStatus
{
    ShaderStatus_OK,
    ShaderStatus_CompileError,
    ShaderStatus_LinkError,
    ShaderStatus_OutOfMemory,
    ShaderStatus_EmptyProgram,
};

class GLShader
{
 protected:
    GLShader(int _id);
    virtual ~GLShader();

 public:
    int getID() const;

 private:
    int id;

    GLShaderStatus compile(const std::vector<std::string>& source);

    friend GLShaderLoader;
};


class GLVertexShader : public GLShader
{
 private:
    GLVertexShader(int _id) : GLShader(_id) {};

 friend GLShaderLoader;
};


class GLFragmentShader : public GLShader
{
 private:
    GLFragmentShader(int _id) : GLShader(_id) {};

 friend GLShaderLoader;
};


class GLProgram
{
 private:
    GLProgram(int _id);
    virtual ~GLProgram();

    void attach(const GLShader&);
    GLShaderStatus link();

 private:
    int id;

 friend GLShaderLoader;
};


class GLShaderLoader
{
 public:
    static GLShaderStatus CreateVertexShader(const std::vector<std::string>&,
                                             GLVertexShader**);
    static GLShaderStatus CreateFragmentShader(const std::vector<std::string>&,
                                               GLFragmentShader**);
    static GLShaderStatus CreateVertexShader(const std::string&,
                                             GLVertexShader**);
    static GLShaderStatus CreateFragmentShader(const std::string&,
                                               GLFragmentShader**);

    static GLShaderStatus CreateProgram(const GLVertexShader& vs,
                                        const GLFragmentShader& fs,
                                        GLProgram**);
    static GLShaderStatus CreateProgram(const std::vector<std::string>& vs,
                                        const std::vector<std::string>& fs,
                                        GLProgram**);
    static GLShaderStatus CreateProgram(const std::string& vs,
                                        const std::string& fs,
                                        GLProgram**);

    static std::string GetInfoLog();
};

#endif // _CELENGINE_GLSHADER_H_
