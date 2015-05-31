/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2015 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scene_credits.h"
#include <graphics/gl_renderer.h>
#include <graphics/graphics.h>
#include <graphics/window.h>
#include <settings/global_settings.h>
#include <common_features/graphics_funcs.h>
#include <data_configs/config_manager.h>
#include <audio/pge_audio.h>

#include <QtDebug>


CreditsScene_misc_img::CreditsScene_misc_img()
{
    x=0;y=0;t.w=0;frmH=0;
}

CreditsScene_misc_img::~CreditsScene_misc_img()
{}

CreditsScene_misc_img::CreditsScene_misc_img(const CreditsScene_misc_img &im)
{
    x=im.x;
    y=im.y;
    t=im.t;
    a=im.a;
    frmH=im.frmH;
}


CreditsScene::CreditsScene() : Scene(Credits)
{
    _waitTimer=5000;
}

CreditsScene::~CreditsScene()
{
    glDisable(GL_TEXTURE_2D);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black background color
    //Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //Reset modelview matrix
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glDeleteTextures( 1, &(background.texture) );

    for(int i=0;i<imgs.size();i++)
    {
        glDisable(GL_TEXTURE_2D);
        glDeleteTextures( 1, &(imgs[i].t.texture) );
    }
    imgs.clear();
}

void CreditsScene::init()
{
    /*****************************Load built-in stuff*******************************/
    glClearColor(float(ConfigManager::setup_CreditsScreen.backgroundColor.red())/255.0f,
                 float(ConfigManager::setup_CreditsScreen.backgroundColor.green())/255.0f,
                 float(ConfigManager::setup_CreditsScreen.backgroundColor.blue())/255.0f, 1.0f);
                // Set background color from file

    if(!ConfigManager::setup_CreditsScreen.backgroundImg.isEmpty())
        background = GraphicsHelps::loadTexture(background, ConfigManager::setup_CreditsScreen.backgroundImg);
    else
        background = GraphicsHelps::loadTexture(background, ":/images/cat_splash.png");
    imgs.clear();
    for(int i=0; i<ConfigManager::setup_CreditsScreen.AdditionalImages.size(); i++)
    {
        if(ConfigManager::setup_CreditsScreen.AdditionalImages[i].imgFile.isEmpty()) continue;

        CreditsScene_misc_img img;
        img.t = GraphicsHelps::loadTexture(img.t, ConfigManager::setup_CreditsScreen.AdditionalImages[i].imgFile);

        img.x = ConfigManager::setup_CreditsScreen.AdditionalImages[i].x;
        img.y = ConfigManager::setup_CreditsScreen.AdditionalImages[i].y;
        img.a.construct(ConfigManager::setup_CreditsScreen.AdditionalImages[i].animated,
                        ConfigManager::setup_CreditsScreen.AdditionalImages[i].frames,
                        ConfigManager::setup_CreditsScreen.updateDelay);

        img.frmH = (img.t.h / ConfigManager::setup_CreditsScreen.AdditionalImages[i].frames);

        imgs.push_back(img);
    }
    /*****************************Load built-in stuff*end***************************/

    /*****************************Load LUA stuff*******************************/
    // onLoad() <- Gives ability to load/init custom stuff


    /*****************************Load LUA stuff*******************************/
}

void CreditsScene::setWaitTime(unsigned int time)
{
    if(time==0)
        _waitTimer=5000;
    else
        _waitTimer=time;
}

void CreditsScene::update()
{
    /******************Update built-in faders and animators*********************/
    Scene::update();
    for(int i=0;i<imgs.size(); i++)
        imgs[i].a.manualTick(uTick);
    /******************Update built-in faders and animators*********************/

    /*****************************Update LUA stuff*******************************/

    //Note: loop function must accept int value which a time ticks loop step.
    // onLoop() <- Independent to time (for message boxes texts, catching events, etc.)
    // onLoop(int ticks) <- If need to sync with a loop time. (For animations, modeling scripts, etc.)


    /*****************************Update LUA stuff*******************************/
}

void CreditsScene::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //Reset modelview matrix
    glLoadIdentity();

    GlRenderer::renderTexture(&background, PGE_Window::Width/2 - background.w/2, PGE_Window::Height/2 - background.h/2);

    for(int i=0;i<imgs.size();i++)
    {
        AniPos x(0,1); x = imgs[i].a.image();
        GlRenderer::renderTexture(&imgs[i].t,
                                  imgs[i].x,
                                  imgs[i].y,
                                  imgs[i].t.w,
                                  imgs[i].frmH, x.first, x.second);
    }

    Scene::render();
}

int CreditsScene::exec()
{
    //Level scene's Loop
    Uint32 start_render;
    float doUpdate_Render=0;

    Uint32 start_wait_timer=SDL_GetTicks();
    while(running)
    {
        //UPDATE Events
        start_render=SDL_GetTicks();
        update();
        render();
        glFlush();
        SDL_GL_SwapWindow(PGE_Window::window);

        if(doExit)
        {
            if(fader.isNull()) fader.setFade(10, 1.0f, 0.01f);
            if(fader.isFull())
                running=false;
        }

        SDL_Event event; //  Events of SDL
        while ( SDL_PollEvent(&event) )
        {
            if(PGE_Window::processEvents(event)!=0) continue;

            switch(event.type)
            {
                case SDL_QUIT:
                    {
                        return -1;
                    }   // End work of program
                break;
                case SDL_KEYDOWN: // If pressed key
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            doExit=true;
                        break;
                        default:
                        break;
                    }
                break;

                case SDL_KEYUP:
                break;
                default: break;
            }
        }

        if( 100.0 / (float)PGE_Window::PhysStep >SDL_GetTicks()-start_render)
        {
            doUpdate_Render = 1000.0/100.0-(SDL_GetTicks()-start_render);
            SDL_Delay( doUpdate_Render );
        }

        if(!doExit)
        {
            if( (SDL_GetTicks()-start_wait_timer) > _waitTimer)
                doExit=true;
        }

    }
    return 0;
}

