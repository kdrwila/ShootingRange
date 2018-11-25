#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

#include "TargetController.h"
#include "Target.h"
#include "Global.h"

TargetController::TargetController(Context* context) :
	LogicComponent(context)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void TargetController::RegisterObject(Context* context)
{
	context->RegisterFactory<TargetController>();
}

void TargetController::Start()
{
	log_->Write(LOG_DEBUG, "Target Controller created");
}

void TargetController::AddPairedController(TargetController * target)
{
	pairedController_ = target;
}

void TargetController::SpawnTarget()
{
	if (!canCreateTargets_)
		return;

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	SetRandomSeed(Random(0, M_MAX_INT));

	Node * parentNode;
	float rand = Random(-1000.0f, 1000.0f);
	if (rand >= 500)
	{
		parentNode = pairedController_->GetNode();
	}
	else
		parentNode = GetNode();

	Node * node = parentNode->CreateChild("Target");
	node->SetPosition(Vector3::ZERO);
	node->SetWorldScale(Vector3::ONE / 75);
	node->SetWorldRotation(Quaternion(0.0f, 90.0f, 90.0f));
	node->SetVar("tag", "box");

	StaticModel * object = node->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/tarcza.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Materials/tarcza.xml"));
	object->SetCastShadows(true);

	float movingSpeed = gameVars_["gameMode"].GetString() == "mode_1" ? .1f : .15f;

	Target * target = node->CreateComponent<Target>();
	target->SetMovingTarget(movingSpeed, rand >= 500 ? true : false);
	target->SetHealth(10.f);
	target->SetController(this);

	RigidBody* body = node->CreateComponent<RigidBody>();
	body->SetCollisionLayer(1);
	body->SetMass(1.f);
	body->SetAngularFactor(Vector3::ZERO);
	body->SetUseGravity(false);
	body->SetTrigger(true);
	body->SetCollisionEventMode(COLLISION_ACTIVE);

	CollisionShape* shape = node->CreateComponent<CollisionShape>();
	shape->SetBox(Vector3::ONE);

	log_->Write(LOG_DEBUG, "Target spawned!");
	canCreateTargets_ = false;
}

void TargetController::SetCanCreateTargets(bool opt)
{
	canCreateTargets_ = opt;
}

void TargetController::RemoveChilds()
{
	Node * node = GetNode()->GetChild("Target", true);
	while (node)
	{
		node->Remove();
		node = GetNode()->GetChild("Target", true);
	}
		
	node = pairedController_->GetNode()->GetChild("Target", true);
	if (node)
		node->Remove();
}