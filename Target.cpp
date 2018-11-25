
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/DecalSet.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Font.h>

#include "Destroy.h"
#include "Target.h"
#include "HumanTargetController.h"
#include "Global.h"

Target::Target(Context* context) :
	LogicComponent(context)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Target::RegisterObject(Context* context)
{
	context->RegisterFactory<Target>();
}

void Target::Start()
{
	log_->Write(LOG_DEBUG, "created target");
	soundNode_ = GetScene()->CreateChild("SoundNode");
	cameraNode_ = GetScene()->GetChild("CameraNode");
	scene_ = GetScene();
}

void Target::RegisterHit(float amount, float hitDistance)
{
	health_ -= amount;

	if (health_ <= 0.0f)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();

		if (ht_isHT_ == false)
		{
			Vector3 pos = GetNode()->GetWorldPosition();

			Node * node = scene_->CreateChild("PointsText");
			node->SetWorldPosition(pos);
			Text3D * text = node->CreateComponent<Text3D>();
			text->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.ttf"), 80);
			text->SetText("+" + (String)(int)ceil(hitDistance));
			text->SetColor(Color::GREEN);
			text->SetTextEffect(TE_STROKE);
			text->SetFaceCameraMode(FaceCameraMode::FC_ROTATE_Y);
			Destroy * destroy = node->CreateComponent<Destroy>();
			destroy->timeAlive_ = 1.5f;

			gameStats_["points"] = gameStats_["points"].GetInt() + (int)ceil(hitDistance);

			GetNode()->SetEnabled(false);
			destroy = GetNode()->CreateComponent<Destroy>();
			destroy->timeAlive_ = 2.0f;

			log_->Write(LOG_DEBUG, "Target destroyed");

			if (controller_ && gameVars_["gameMode"].GetString() != "none")
			{
				controller_->SetCanCreateTargets(true);
			}
		}
		else if(ht_isActive_ == true)
		{
			HumanTargetController * ht_controller = scene_->GetComponent<HumanTargetController>();
			ht_controller->SetCanShowTarget(true);
			ht_isActive_ = false;

			GetNode()->Translate(Vector3(1.5f, .0f, .0f));

			if (gameStats_["m3_targetsLeft"].GetInt() == 1)
			{
				ht_controller->EndGame();
			}

			if (ht_isVictim_ == true)
			{
				ht_pointsIsActive_ = true;
				ht_pointsTimeLeft_ = 3.0f;
				Node * node = GetNode()->GetChild("Points");
				node->SetEnabled(true);
				gameVars_["timeLeft"] = gameVars_["timeLeft"].GetFloat() + 10.0f;
			}
		}

		Sound* sound = cache->GetResource<Sound>("Sounds/metal.wav");

		SoundSource* soundSource = soundNode_->CreateComponent<SoundSource>();
		soundSource->Play(sound);
		soundSource->SetGain(1.0f);
		
		gameStats_["targetsDestroyed"] = gameStats_["targetsDestroyed"].GetInt() + 1;
		gameStats_["m3_targetsLeft"] = gameStats_["m3_targetsLeft"].GetInt() - 1;
	}
}

void Target::SetMovingTarget(float speed, bool direction)
{
	movingDirection_ = direction;
	movingSpeed_ = speed;

	SubscribeToEvent(GetNode(), E_NODECOLLISIONSTART, URHO3D_HANDLER(Target, HandleNodeCollisionStart));
}

void Target::SetHealth(float amount)
{
	health_ = amount;
}

void Target::SetController(TargetController * controller)
{
	controller_ = controller;
}

void Target::FixedUpdate(float timeStep)
{
	if (ht_isActive_)
	{
		if (ht_timeLeft_ > 0.0f)
		{
			ht_timeLeft_ -= timeStep;
		}
		else
		{
			HumanTargetController * ht_controller = scene_->GetComponent<HumanTargetController>();
			ht_controller->SetCanShowTarget(true);
			ht_isActive_ = false;

			GetNode()->Translate(Vector3(1.5f, .0f, .0f));

			if (gameStats_["m3_targetsLeft"].GetInt() == 1)
			{
				ht_controller->EndGame();
			}

			gameStats_["m3_targetsLeft"] = gameStats_["m3_targetsLeft"].GetInt() - 1;
		}
	}

	if (ht_pointsIsActive_)
	{
		if (ht_pointsTimeLeft_ > 0.0f)
		{
			ht_pointsTimeLeft_ -= timeStep;
		}
		else
		{
			Node * node = GetNode()->GetChild("Points");
			node->SetEnabled(false);
			ht_pointsIsActive_ = false;
		}
	}

	if (movingSpeed_ > .0f)
	{
		GetNode()->Translate(Vector3(.0f, .0f, movingDirection_ ? -movingSpeed_ : movingSpeed_), TS_WORLD);
	}

	if (respawnCooldown_ > .0f)
		respawnCooldown_ -= timeStep;
	else if(respawn_)
	{
		if (gameVars_["gameMode"].GetString() != "none")
		{
			respawn_ = false;
			respawnCooldown_ = .0f;
			GetNode()->Remove();

			if(controller_)
				controller_->SetCanCreateTargets(true);
		}
	}
}

void Target::HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData)
{
	if (movingSpeed_ <= .0f)
		return;

	using namespace NodeCollisionStart;

	Node * hitNode = (Node*)eventData[P_OTHERNODE].GetPtr();

	log_->Write(LOG_DEBUG, "Found collision with: " + hitNode->GetName());

	if (hitNode->GetName() == "TargetTrigger")
	{
		movingDirection_ = !movingDirection_;

		contactCount_++;

		if (gameVars_["gameMode"].GetString() == "mode_2" && contactCount_ > 1)
		{
			GetNode()->SetWorldPosition(Vector3(1000.0f, 1000.0f, 1000.0f));
			respawnCooldown_ = 1.0f;
			respawn_ = true;
		}
	}
}

void Target::HT_SetVictim(bool toggle)
{
	ht_isVictim_ = toggle;
}

void Target::HT_SetHT(bool toggle)
{
	ht_isHT_ = toggle;
	ht_timeLeft_ = 3.0f;
}

void Target::HT_SetActive(bool toggle)
{
	ht_isActive_ = toggle;
}