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
#include "nxNexteroidsGameLogicEventListener.hpp"

#include <nxActor/nxBulletActor.hpp>
#include <nxActor/nxRockActor.hpp>
#include <nxEvent/nxEventManager.hpp>
#include <nxGameView/nxNexteroidsGameView.hpp>

bool nxNexteroidsGameLogicEventListener::HandleEvent( nxIEventData const & event )
{
	if(event.VGetEventType() == NX_EVENT_RequestNewGame)
	{
		log(NX_LOG_DEBUG, "New game event received");
//		m_pGameLogic->VChangeState(NX_GS_LoadingGameEnvironment);
		m_pGameLogic->VChangeState(NX_GS_WaitingForPlayers);
		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_GameState)
	{
        nxGameStateEventData* gameStateEvent = (nxGameStateEventData*)(&event);
		if(gameStateEvent->GetState() == NX_GS_LoadingGameEnvironment)
		{
		}
		else if(gameStateEvent->GetState() == NX_GS_LoadedLevel00)
		{
			m_pGameLogic->VBuildLevel00();
		}

		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_RequestNewActor)
	{
		//Check to make sure request is worthy
		const nxRequestNewActorEventData & castEvent =
			static_cast< const nxRequestNewActorEventData & >( event );
		//Validate event and get a new actor id and generate a new actor event.
		nxIActor* pActor = castEvent.GetActor();
		pActor->VCreate();

		const nxIEventDataPtr eventPtr( NX_NEW nxNewActorEventData(pActor));
		nxEventManager::GetInstance().VQueueEvent( eventPtr );

		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_NewActor)
	{
		const nxNewActorEventData & castEvent =
			static_cast< const nxNewActorEventData & >( event );

		shared_ptr<nxIActor> pActor(castEvent.GetActor()->VClone());

		m_pGameLogic->VAddActor(pActor);

		return true;

	}
	else if(event.VGetEventType() == NX_EVENT_Thrust)
	{
		const nxThrustEventData & castEvent =
			static_cast< const nxThrustEventData & >( event );
		shared_ptr<nxIActor> pActor = m_pGameLogic->VGetActor(castEvent.GetActorId());

		if( pActor )
		{
			m_pGameLogic->VGetPhysics()->VApplyThrottle(pActor->VGetId(), castEvent.GetThrustAmount());
		}
		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_Steer)
	{
		const nxSteerEventData & castEvent =
			static_cast< const nxSteerEventData & >( event );
		shared_ptr<nxIActor> pActor = m_pGameLogic->VGetActor(castEvent.GetActorId());

		if( pActor )
		{
			m_pGameLogic->VGetPhysics()->VApplySteering(pActor->VGetId(), castEvent.GetSteerAmount());
		}
		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_FireWeapon)
	{
		const nxFireWeaponEventData & castEvent =
			static_cast< const nxFireWeaponEventData & >( event );
		shared_ptr<nxIActor> pActor = m_pGameLogic->VGetActor(castEvent.GetActorId());

		if( pActor )
		{
			float r = 30.0f;
			float bulletRadius = 1.0f;
			pActor->VSetRadius(bulletRadius);
			r += bulletRadius;
			float bulletSpeed = 40000.0f;
			nxPoint3 bulletPos(pActor->VGetPos().GetX() + (r * cos(pActor->VGetOrientation())),
								pActor->VGetPos().GetY() + (r * sin(pActor->VGetOrientation())),
								0);
			nxReal bulletRot = pActor->VGetOrientation();
			shared_ptr<nxBulletActor> pBullet(NX_NEW nxBulletActor(bulletRot, bulletPos, bulletRadius, bulletSpeed));
			const nxActorId actorId = m_pGameLogic->GetNewActorId();
			pBullet->VSetId(actorId);
			nxColour bulletColour;
			bulletColour.r = 1;
			bulletColour.g = 1;
			bulletColour.b = 0; //Server is always green! Best colour
			pBullet->VSetColour(bulletColour);
			pBullet->VSetViewId(-1); //Don't think I need this?

			const nxIEventDataPtr eventPtr( NX_NEW nxRequestNewActorEventData(pBullet));
			nxEventManager::GetInstance().VQueueEvent(eventPtr);
		}
		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_PhysCollision)
	{
		const nxPhysCollisionEventData & castEvent =
			static_cast< const nxPhysCollisionEventData & >( event );

		shared_ptr<nxIActor> actorA = m_pGameLogic->VGetActor(castEvent.GetActorIdA());
		shared_ptr<nxIActor> actorB = m_pGameLogic->VGetActor(castEvent.GetActorIdB());

		if( actorA && actorB )
		{
			if( 
					((actorA->VGetType() == NX_ACTOR_ROCK) &&
					 (actorB->VGetType() == NX_ACTOR_BULLET)) ||
					((actorA->VGetType() == NX_ACTOR_BULLET) &&
					 (actorB->VGetType() == NX_ACTOR_ROCK))
			  )
			{
				shared_ptr<nxIActor> rock, bullet;
				//Rock hit bullet
				if(actorA->VGetType() == NX_ACTOR_BULLET)
				{
					bullet = actorA;
					rock = actorB;
				}
				else
				{
					rock = actorA;
					bullet = actorB;
				}
				//Now register hit and kill actor. 
				m_pGameLogic->VRegisterHit(rock->VGetId(), bullet->VGetId());

				const nxIEventDataPtr eventPtr( NX_NEW nxActorDeathEventData(bullet->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(eventPtr);

				nxPoint3 rockPos = rock->VGetPos();
				nxReal rockRot = rock->VGetOrientation();
				nxReal rockRadius = rock->GetRadius();
//				int rockType = ((nxRockActor*)(rock.get()))->VGetRockType();

				const nxIEventDataPtr rockEventPtr( NX_NEW nxActorDeathEventData(rock->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(rockEventPtr);

				//TODO:Blowing up sounds, effects, what have you.
				//If not a small rock, make smaller ones.
				if(rockRadius >= 20)
				{
					//Spawn 2 rocks in place
					//1st rock
					rockPos.SetX(rockPos.GetX() + 20 ); //TODO:Make more random
					nxReal rockSpeed = 4;
					nxReal rockRadius = 10; //Make a smaller rock
					shared_ptr<nxRockActor> pRock(NX_NEW nxRockActor(rockRot, rockPos, rockRadius, rockSpeed));
					nxColour rockColour;
					rockColour.r = 1;
					rockColour.g = 1;
					rockColour.b = 1; //Server is always green! Best colour
					pRock->VSetColour(rockColour);
					pRock->VSetViewId(-1); //Don't need this I think
					pRock->VSetRockType(0);
					const nxActorId rockActorId = m_pGameLogic->GetNewActorId();
					pRock->VSetId(rockActorId);
					const nxIEventDataPtr newRockEventPtr( NX_NEW nxRequestNewActorEventData(pRock));
					nxEventManager::GetInstance().VQueueEvent(newRockEventPtr);
					
					//2nd rock
					rockPos.SetX(rockPos.GetX() - 40); //TODO:Make more random
					rockSpeed = 4;
					rockRadius = 10;  //Make a smaller rock
					shared_ptr<nxRockActor> pRock2(NX_NEW nxRockActor(rockRot, rockPos, rockRadius, rockSpeed));
					nxColour rockColour2;
					rockColour2.r = 1;
					rockColour2.g = 1;
					rockColour2.b = 1; //Server is always green! Best colour
					pRock2->VSetColour(rockColour2);
					pRock2->VSetViewId(-1); //Don't need this I think
					pRock2->VSetRockType(0);
					const nxActorId rock2ActorId = m_pGameLogic->GetNewActorId();
					pRock2->VSetId(rock2ActorId);
					const nxIEventDataPtr newRock2EventPtr( NX_NEW nxRequestNewActorEventData(pRock2));
					nxEventManager::GetInstance().VQueueEvent(newRock2EventPtr);
				}
			}
			else if( 
					((actorA->VGetType() == NX_ACTOR_BULLET) &&
					 (actorB->VGetType() == NX_ACTOR_SPACESHIP)) ||
					((actorA->VGetType() == NX_ACTOR_SPACESHIP) &&
					 (actorB->VGetType() == NX_ACTOR_BULLET))
			  )
			{
				shared_ptr<nxIActor> ship, bullet;
				//Rock hit bullet
				if(actorA->VGetType() == NX_ACTOR_BULLET)
				{
					bullet = actorA;
					ship = actorB;
				}
				else
				{
					ship = actorA;
					bullet = actorB;
				}
				const nxIEventDataPtr bulletEventPtr( NX_NEW nxActorDeathEventData(bullet->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(bulletEventPtr);

				const nxIEventDataPtr shipEventPtr( NX_NEW nxActorDeathEventData(ship->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(shipEventPtr);
			}
			else if( 
					((actorA->VGetType() == NX_ACTOR_ROCK) &&
					 (actorB->VGetType() == NX_ACTOR_SPACESHIP)) ||
					((actorA->VGetType() == NX_ACTOR_SPACESHIP) &&
					 (actorB->VGetType() == NX_ACTOR_ROCK))
			  )
			{
				shared_ptr<nxIActor> ship, rock;
				//Rock hit rock
				if(actorA->VGetType() == NX_ACTOR_ROCK)
				{
					rock = actorA;
					ship = actorB;
				}
				else
				{
					ship = actorA;
					rock = actorB;
				}
				const nxIEventDataPtr rockEventPtr( NX_NEW nxActorDeathEventData(rock->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(rockEventPtr);

				const nxIEventDataPtr shipEventPtr( NX_NEW nxActorDeathEventData(ship->VGetId()));
				nxEventManager::GetInstance().VQueueEvent(shipEventPtr);

			}
		}
		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_ActorDeath)
	{
		const nxActorDeathEventData & castEvent =
			static_cast< const nxActorDeathEventData & >( event );

		nxActorId aid = castEvent.GetActorId();

		shared_ptr<nxIActor> pActor = m_pGameLogic->VGetActor(aid);
		if(pActor)
		{
			//Spawn another one!
//			m_pGameLogic->SpawnActor(pActor->VGetType());
			m_pGameLogic->VRemoveActor(aid);
		}

		return true;
	}
	else if(event.VGetEventType() == NX_EVENT_ActorMoved)
	{
		const nxActorMovedEventData & castEvent =
			static_cast< const nxActorMovedEventData & >( event );

		m_pGameLogic->VMoveActor(castEvent.GetActorId(), castEvent.GetPos(), castEvent.GetOrientation());
	}
	else if(event.VGetEventType() == NX_EVENT_RemoteClient)
	{
		const nxRemoteClientEventData & castEvent =
			static_cast< const nxRemoteClientEventData & >( event );

		if(m_pGameLogic->IsProxy())
		{
			//We need to create the real game view.
			//Now this will listen to all events from server, we can just kick back.:D
			int viewId = m_pGameLogic->GetIpInt();
			shared_ptr<nxIGameView> gameView(NX_NEW nxNexteroidsGameView());
			gameView->Init();
			m_pGameLogic->VPopView();
//			m_pGameLogic->VAddView(gameView, -1); //Should be set automagically over net!
			m_pGameLogic->VAddViewExplicit(gameView, viewId, -1); //Should be set automagically over net!
		}	
		else
		{
			//Set the number of players connected up!
			m_pGameLogic->SetNRemotePlayers(m_pGameLogic->GetNRemotePlayers() + 1);
			m_pGameLogic->VAddConnectedClientId(castEvent.GetIpInt());
		}
	}
	return false;
}

