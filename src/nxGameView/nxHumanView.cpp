/**
Nexteroids - A cross platform, networked asteroids game.
Copyright (C) 2010 Jonathan Frawley

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
#include "nxHumanView.hpp"

#include <nxGameApp/nxNexteroidsGameApp.hpp>

nxHumanView::nxHumanView()
{
    m_pProcessManager = NX_NEW nxProcessManager();
}

nxHumanView::~nxHumanView()
{
    NX_SAFE_DELETE(m_pProcessManager);
}

bool nxHumanView::Init()
{
	m_Input = nxNexteroidsGameApp::GetInstance().VGetInput();
    return true;
}

void nxHumanView::VOnRender()
{
	//Update all attached screens.
	for(nxScreenList::iterator i=m_Screens.begin();
			i!=m_Screens.end(); ++i)
	{
		(*i)->VOnRender();
	}

//	SDL_GL_SwapBuffers();
}

void nxHumanView::VHandleKeyboardCommands(SDL_Event event)
{
	if (event.type == SDL_KEYDOWN)
	{
		if (event.key.keysym.sym == SDLK_ESCAPE)
		{
			nxNexteroidsGameApp::GetInstance().SetShuttingDown();
		}
		if (event.key.keysym.sym == SDLK_f)
		{
			if (event.key.keysym.mod & KMOD_CTRL)
			{

				nxNexteroidsGameApp::GetInstance().ToggleFullscreen();
			}
		}
	}
	else if(event.type == SDL_QUIT)
	{
		nxNexteroidsGameApp::GetInstance().SetShuttingDown();
	}
}

void nxHumanView::VOnUpdate(int deltaMilliseconds)
{
	m_pProcessManager->UpdateProcesses(deltaMilliseconds);
	for(nxScreenList::iterator i=m_Screens.begin();
			i!=m_Screens.end(); ++i)
	{
		(*i)->VOnUpdate(deltaMilliseconds);
	}

	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		VHandleKeyboardCommands(event);

		// After we have manually checked user input with SDL for
		// any attempt by the user to halt the application we feed
		// the input to Guichan by pushing the input to the Input
		// object.
		m_Input->pushInput(event);
	}
}
