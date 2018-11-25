#pragma once

#include "TargetController.h"

#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Graphics/Renderer.h>

using namespace Urho3D;

class Target : public LogicComponent
{
	URHO3D_OBJECT(Target, LogicComponent);

public:
	/// Construct.
	Target(Context* context);

	static void RegisterObject(Context* context);
	virtual void Start();

	void RegisterHit(float amount, float hitDistance);
	void SetMovingTarget(float speed, bool direction);
	void SetHealth(float amount);
	void SetController(TargetController * controller);

	void FixedUpdate(float timeStep);
	void HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData);

	void HT_SetVictim(bool toggle);
	void HT_SetHT(bool toggle);
	void HT_SetActive(bool toggle);

protected:

	float health_ = 1000.f;
	bool movingDirection_ = false;
	float movingSpeed_ = .0f;
	unsigned int contactCount_ = 0;
	float respawnCooldown_ = .0f;
	bool respawn_ = false;
	bool isActive = false;

	bool ht_isVictim_ = false;
	bool ht_isHT_ = false;
	bool ht_isActive_ = false;
	float ht_timeLeft_ = 0.0f;
	bool ht_pointsIsActive_ = false;
	float ht_pointsTimeLeft_ = 0.0f;

private:
	
	Node * soundNode_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> scene_;

	TargetController * controller_;
};