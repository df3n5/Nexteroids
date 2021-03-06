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
#include "nxPhysics.hpp"

#include <nxGameApp/nxGameApp.hpp>
#include <nxCore/nxMath.hpp>
#include <nxCore/nxTypes.hpp>
#include <nxActor/nxBulletActor.hpp>
#include <nxEvent/nxPhysCollisionEventData.hpp>
#include <nxEvent/nxActorMovedEventData.hpp>
#include <nxEvent/nxActorDeathEventData.hpp>
#include <nxEvent/nxEventManager.hpp>

nxPhysics::nxPhysics()
{
	// Define the gravity vector.
	b2Vec2 gravity(0.0f, 0.0f);

	// Do we want to let bodies sleep?
	bool doSleep = true;

	// Construct a world object, which will hold and simulate the rigid bodies.
	m_World = NX_NEW b2World(gravity, doSleep);
	m_World->SetContactListener(this);
}

nxPhysics::~nxPhysics()
{
	NX_SAFE_DELETE(m_World);
}

void nxPhysics::VInit()
{

}

vector<b2Vec2> nxPhysics::ConvertPointToVec2(vector<nxPoint3> points)
{
	vector<b2Vec2> result;

	vector<nxPoint3>::iterator it, itEnd;
	for(it = points.begin(), itEnd = points.end() ; it != itEnd ; it++)
	{
		b2Vec2 vec((*it).GetX(), (*it).GetY());
		result.push_back(vec);
	}
	return result;
}

void nxPhysics::VCreateActor(shared_ptr<nxIActor> actor)
{
	if(actor->VGetType() == NX_ACTOR_SPACESHIP)
	{
		b2Vec2 points[3];
		float shipSize = 20.f;

		points[0].x = 1.0f * shipSize;
		points[0].y = 0.0f * shipSize;
		points[1].x = -1.0f * shipSize;
		points[1].y = 0.5f * shipSize;
		points[2].x = -1.0f * shipSize;
		points[2].y = -0.5f * shipSize;

		b2BodyDef bodyDef;

		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(actor->VGetPos().GetX(), actor->VGetPos().GetY());
		bodyDef.angle = actor->VGetOrientation();
		bodyDef.userData = actor.get(); 
		b2Body* body = m_World->CreateBody(&bodyDef);
		m_Bodies.push_back(body);

		//Create polygon shape
		b2PolygonShape dynamicPolygon;
		dynamicPolygon.Set(points, 3);

		//Create custom fixture
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &dynamicPolygon;
		fixtureDef.density = 1.0f;
		fixtureDef.friction = 0.3f;

		body->CreateFixture(&fixtureDef);
	}
	else if(actor->VGetType() == NX_ACTOR_BULLET)
	{
			b2CircleShape shape;
			shape.m_radius = 1.0f;

			b2BodyDef bd;

			bd.userData = actor.get();
			bd.type = b2_dynamicBody;
			bd.position.Set(actor->VGetPos().GetX(), actor->VGetPos().GetY());

			b2Body* body = m_World->CreateBody(&bd);
			m_Bodies.push_back(body);

			body->CreateFixture(&shape, 1.0f);


			float bulletSpeed = 90000000000;
			float angle = actor->VGetOrientation();
			b2Vec2 force;
			force.x = bulletSpeed * cos(angle);
			force.y = bulletSpeed * sin(angle);

			body->ApplyForce(force, body->GetWorldCenter());
	}
	else if(actor->VGetType() == NX_ACTOR_ROCK)
	{
		nxRockActor* castActor = (nxRockActor*)actor.get();

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(castActor->VGetPos().GetX(), castActor->VGetPos().GetY());
		bodyDef.angle = castActor->VGetOrientation();
		bodyDef.userData = castActor;
		b2Body* body = m_World->CreateBody(&bodyDef);
		m_Bodies.push_back(body);

		float r = 10.f; //TODO Not hard code this.
		float rockRadius = 1.0f;

		//Setup Points, this is the tricky part...
		vector<b2Vec2> vecPoints = ConvertPointToVec2(castActor->GetPoints());
		b2PolygonShape polygonShape;
		polygonShape.Set(&(vecPoints.front()), vecPoints.size());

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &polygonShape;
		fixtureDef.density = 0.3f;
		b2Fixture* myFixture = body->CreateFixture(&fixtureDef);

		//This ensures it won't go to sleep.

		//Apply force to get rock moving
		float rockSpeed = castActor->VGetSpeed();
		//			float rockSpeed = 3000000;
		float angle = castActor->VGetOrientation();
		b2Vec2 force;
		force.x = rockSpeed * cos(angle);
		force.y = rockSpeed * sin(angle);
		//			body->ApplyForce(force, body->GetWorldCenter());
		body->SetLinearVelocity(force);

		int maxAngularVel = NX_PI;
		float angularVel = (float) (rand() % maxAngularVel) / 16;
		body->SetAngularVelocity(angularVel);
	}
}

void nxPhysics::VApplyThrottle(nxActorId actorId, float throttle)
{
	vector<b2Body*>::iterator it, itEnd;
	for(it = m_Bodies.begin(), itEnd = m_Bodies.end() ; it != itEnd ; it++)
	{
		b2Body* body = (*it);
		nxIActor* actor = (nxIActor*)(body->GetUserData());
		if(actor->VGetId() == actorId)
		{
			float32 angle = body->GetAngle();
			
			b2Vec2 linear = body->GetLinearVelocity();

			float maxSpeed = 100;

			b2Vec2 force;
			float speed = 5.0f;
			force.x = throttle*speed*cos(angle);
			force.y = throttle*speed*sin(angle);
			body->SetLinearVelocity(force);
			body->SetAwake(true);
		}
	}
}

void nxPhysics::VApplySteering(nxActorId actorId, float steering)
{
	vector<b2Body*>::iterator it, itEnd;
	for(it = m_Bodies.begin(), itEnd = m_Bodies.end() ; it != itEnd ; it++)
	{
		b2Body* body = (*it);
		nxIActor* actor = (nxIActor*)(body->GetUserData());
		if(actor->VGetId() == actorId)
		{
			float angular = body->GetAngularVelocity();
			float angularDelta = steering * (NX_PI_OVER_16);
			float maxSpeed = 2;

			if(angular > maxSpeed)
			{
				continue;
			}

			body->SetAngularVelocity(angularDelta);
		}
	}
}

void nxPhysics::VOnUpdate(float timeStep)
{
	int32 velocityIterations = 8;
	int32 positionIterations = 3;

	// Instruct the world to perform a single step of simulation.
	// It is generally best to keep the time step and iterations fixed.
	m_World->Step(timeStep, velocityIterations, positionIterations);

	// Clear applied body forces. We didn't apply any forces, but you
	// should know about this function.
	m_World->ClearForces();

	//Reconcile actor and physics positions
	vector<b2Body*>::iterator it, itEnd;
	for(it = m_Bodies.begin(), itEnd = m_Bodies.end() ; it != itEnd ; it++)
	{

		b2Body* body = (*it);
		b2Vec2 position = body->GetPosition();
		float32 angle = body->GetAngle();
		nxIActor* actor = (nxIActor*)(body->GetUserData());
		if ( (actor->VGetPos().GetX() != position.x) ||
				(actor->VGetPos().GetY() != position.y) )
		{
			bool destroy = true;
			//Wrap if necessary
			if( (position.x < 0) )
			{
				b2Vec2 newPos( position );
				newPos.x = NX_SCREEN_WIDTH;
				body->SetTransform(newPos, body->GetAngle());
				actor->VSetPos(newPos);
			}
			else if( (position.y < 0) )
			{
				b2Vec2 newPos( position );
				newPos.y = NX_SCREEN_HEIGHT;
				body->SetTransform(newPos, body->GetAngle());
				actor->VSetPos(newPos);
			}
			else if( (position.x > NX_SCREEN_WIDTH) )
			{
				b2Vec2 newPos( position );
				newPos.x = 0;
				body->SetTransform(newPos, body->GetAngle());
				actor->VSetPos(newPos);
			}
			else if( (position.y > NX_SCREEN_HEIGHT) )
			{
				b2Vec2 newPos( position );
				newPos.y = 0;
				body->SetTransform(newPos, body->GetAngle());
				actor->VSetPos(newPos);
			}
			else
			{
				destroy = false;
				//Box2d has moved object, need to generate a moved actor event.
				actor->VSetPos(position);
				actor->VSetOrientation(angle);
			}

			if(destroy && (actor->VGetType() == NX_ACTOR_BULLET))
			{
				//Kill bullet if it goes out of bounds, not having bullets wrapping around level, sadistic stuff that is.
				const nxIEventDataPtr eventPtr( NX_NEW nxActorDeathEventData(actor->VGetId()) );
				nxEventManager::GetInstance().VQueueEvent(eventPtr);
			}
			else
			{
				const nxIEventDataPtr eventPtr( NX_NEW nxActorMovedEventData(actor->VGetId(), actor->VGetPos(), actor->VGetOrientation()));
				nxEventManager::GetInstance().VQueueEvent(eventPtr);
			}
		}
	}
}

void nxPhysics::BeginContact(b2Contact* contact)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();
	b2Body* bodyA = fixtureA->GetBody();
	b2Body* bodyB = fixtureB->GetBody();
	nxIActor* actorA = (nxIActor*)bodyA->GetUserData();
	nxIActor* actorB = (nxIActor*)bodyB->GetUserData();

	const nxIEventDataPtr collEventPtr( NX_NEW nxPhysCollisionEventData(actorA->VGetId(), actorB->VGetId()));
	nxEventManager::GetInstance().VQueueEvent(collEventPtr);
}
