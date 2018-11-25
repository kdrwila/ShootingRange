#pragma once

#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

class TargetController : public LogicComponent
{
	URHO3D_OBJECT(TargetController, LogicComponent);

public:
	/// Construct.
	TargetController(Context* context);

	static void RegisterObject(Context* context);
	virtual void Start();

	void AddPairedController(TargetController * target);
	void SpawnTarget();
	void SetCanCreateTargets(bool opt);
	void RemoveChilds();

private:

	bool canCreateTargets_ = false;
	TargetController * pairedController_;
};