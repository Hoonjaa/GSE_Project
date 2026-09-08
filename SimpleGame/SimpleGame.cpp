/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "Renderer.h"
#include "Prototype.h"
#include <iostream>
#include <memory>
#include <cstdint>

namespace
{
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Prototype> game;
    std::uint32_t previousTick=0;

    void RenderScene()
    {
        if (!renderer || !game) return;
        game->Render(*renderer);
        glutSwapBuffers();
    }
    void Resize(int w,int h)
    {
        if (renderer) renderer->Resize(w,h);
    }
    void Close()
    {
        // Free GPU objects while this window's OpenGL context is still current.
        game.reset();
        renderer.reset();
    }
    void Tick(int)
    {
        if (!game || !renderer) return;
        const auto now=static_cast<std::uint32_t>(glutGet(GLUT_ELAPSED_TIME));
        const double dt=static_cast<double>(now-previousTick)/1000.0;
        previousTick=now;
        game->Update(dt,renderer->Width(),renderer->Height());
        glutPostRedisplay();
        glutTimerFunc(16,Tick,0);
    }
    void KeyDown(unsigned char key,int,int)
    {
        if (key==27) {glutLeaveMainLoop();return;}
        if (game) game->Key(key,true);
    }
    void KeyUp(unsigned char key,int,int)
    {
        if (game) game->Key(key,false);
    }
    void Wheel(int,int direction,int,int)
    {
        if (game) game->Zoom(direction);
    }
    void Entry(int state)
    {
        if (game && state==GLUT_LEFT) game->ClearInput();
    }
    void Visibility(int state)
    {
        if (game && state!=GLUT_VISIBLE) game->ClearInput();
    }
}

int main(int argc,char** argv)
{
    glutInit(&argc,argv);
    glutInitContextVersion(3,3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_MULTISAMPLE);
    glutInitWindowSize(1280,800);
    glutInitWindowPosition(80,50);
    const int window=glutCreateWindow("Uncharted - OpenGL Rendering Prototype");
    if (window<=0) return 1;
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glewExperimental=GL_TRUE;
    const GLenum status=glewInit();
    if (status!=GLEW_OK || !GLEW_VERSION_3_3) {
        std::cerr<<"OpenGL 3.3 is required. GLEW: "<<glewGetErrorString(status)<<'\n';
        glutDestroyWindow(window);
        return 1;
    }
    // GLEW probes may leave GL_INVALID_ENUM on a core context.
    while (glGetError()!=GL_NO_ERROR) {}
    std::cout<<"OpenGL: "<<glGetString(GL_VERSION)<<"\n"
        <<"WASD: move | Space: boost | E: board/land | Wheel: zoom\n"
        <<"G: chunk grid | H: HUD | R: return home | Escape: exit\n";
    renderer.reset(new Renderer(1280,800));
    if (!renderer->IsInitialized()) {
        std::cerr<<"Renderer initialization failed. Check the shader error above.\n";
        renderer.reset();
        glutDestroyWindow(window);
        return 1;
    }
    game.reset(new Prototype());
    game->Update(0,1280,800);
    glutDisplayFunc(RenderScene);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutMouseWheelFunc(Wheel);
    glutEntryFunc(Entry);
    glutVisibilityFunc(Visibility);
    glutCloseFunc(Close);
    glutIgnoreKeyRepeat(1);
    previousTick=static_cast<std::uint32_t>(glutGet(GLUT_ELAPSED_TIME));
    glutTimerFunc(16,Tick,0);
    glutMainLoop();
    // Close callback released the renderer before the window was destroyed.
    return 0;
}

