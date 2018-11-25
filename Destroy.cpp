
#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

#include "Destroy.h"

Destroy::Destroy(Context* context) :
	LogicComponent(context)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Destroy::RegisterObject(Context* context)
{
	context->RegisterFactory<Destroy>();
}

void Destroy::Start()
{

}

void Destroy::FixedUpdate(float timeStep)
{
	if (timeAlive_ > 0.0f)
	{
		timeAlive_ -= timeStep;
	}
	else
	{
		GetNode()->Remove();
	}
}