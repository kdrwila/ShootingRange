#pragma once

#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

class Destroy : public LogicComponent
{
	URHO3D_OBJECT(Destroy, LogicComponent);

public:
	/// Construct.
	Destroy(Context* context);

	static void RegisterObject(Context* context);
	virtual void Start();

	void FixedUpdate(float timeStep);

	float timeAlive_ = 3.0f;
};