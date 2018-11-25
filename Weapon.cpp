#include <cmath>

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
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Window.h>

#include "Weapon.h"
#include "Global.h"
#include "Target.h"
#include "HumanTargetController.h"

Weapon::Weapon(Context* context) :
	LogicComponent(context)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Weapon::RegisterObject(Context* context)
{
	context->RegisterFactory<Weapon>();
}

void Weapon::Setup(Vector<Window *> * wh, Sprite * wc, Window * rw, Text * rt)
{
	wh_ = wh;
	weaponCrosshair_ = wc;
	resultWindow_ = rw;
	resultText_ = rt;
}

void Weapon::Start()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	ui->GetRoot()->SetDefaultStyle(style);

	pointsText_ = ui->GetRoot()->CreateChild<Text>();
	pointsText_->SetText(
		"Points: 0"
	);
	pointsText_->SetFont(cache->GetResource<Font>("Fonts/Prototype.ttf"), 20);
	pointsText_->SetTextAlignment(HA_LEFT);
	pointsText_->SetPosition(10, 10);

	timerText_ = ui->GetRoot()->CreateChild<Text>();
	timerText_->SetText(
		"Time left: 0s"
	);
	timerText_->SetFont(cache->GetResource<Font>("Fonts/Prototype.ttf"), 20);
	timerText_->SetTextAlignment(HA_RIGHT);
	timerText_->SetPosition(-10, 10);
	timerText_->SetAlignment(HA_RIGHT, VA_TOP);

	cameraNode_ = GetScene()->GetChild("CameraNode");
	shotLightNode_ = GetNode()->GetChild("WeaponShotLightNode");
	shotFireNode_ = GetNode()->GetChild("WeaponShotFireNode");
	soundNode_ = GetScene()->CreateChild("SoundNode");
}

void Weapon::FixedUpdate(float timeStep)
{
	timeCounter_ += timeStep;

	Input* input = GetSubsystem<Input>();

	if (input->GetMouseButtonDown(MOUSEB_LEFT) && !input->IsMouseVisible())
	{
		if (canShootNextBullet)
		{
			if (timeCounter_ - lastShoot_ > weaponsData_[gameVars_["selectedWeapon"].GetString()]["shootingInterval"].GetFloat())
			{
				CreateBullet();

				lastShoot_ = timeCounter_;

				ResourceCache* cache = GetSubsystem<ResourceCache>();
				Sound* sound = cache->GetResource<Sound>(weaponsData_[gameVars_["selectedWeapon"].GetString()]["sound"].GetString());

				SoundSource* soundSource = GetScene()->CreateComponent<SoundSource>();
				soundSource->Play(sound);
				soundSource->SetGain(.75f);

				FindHit();

				if (!weaponsData_[gameVars_["selectedWeapon"].GetString()]["automatic"].GetBool())
				{
					canShootNextBullet = false;
				}
			}

			if (lastUpdateShoot_ == true && burstCounter_ < weaponsData_[gameVars_["selectedWeapon"].GetString()]["maxSpreadTime"].GetFloat())
			{
				burstCounter_ += timeStep;
			}
			lastUpdateShoot_ = true;
		}
	}
	else
	{
		canShootNextBullet = true;
		lastUpdateShoot_ = false;
		if(burstCounter_ > 0)
			burstCounter_ -= timeStep * 2;
	}

	if (controls_.IsDown(CTRL_PRIMARY))
	{
		gameVars_["selectedWeapon"] = gameVars_["primaryWeapon"];
		ChangeWeapon(gameVars_["selectedWeapon"].GetString());
	}
	if (controls_.IsDown(CTRL_SECONDARY))
	{
		gameVars_["selectedWeapon"] = gameVars_["secondaryWeapon"];
		ChangeWeapon(gameVars_["selectedWeapon"].GetString());
	}

	if (shotFireEnabledTime_ > 0.05f)
	{
		shotLightNode_->SetEnabled(false);
		shotFireNode_->SetEnabled(false);
	}
	else
		shotFireEnabledTime_ += timeStep;

	// Time Control

	if (gameVars_["gameMode"].GetString() == "mode_3")
	{
		gameVars_["timeLeft"] = gameVars_["timeLeft"].GetFloat() + timeStep;
	}
	else
	{
		if (gameVars_["timeLeft"].GetFloat() > 0.0f)
		{
			gameVars_["timeLeft"] = gameVars_["timeLeft"].GetFloat() - timeStep;
		}
		else if (gameVars_["gameMode"].GetString() != "none")
		{
			gameVars_["timeLeft"] = 0.0f;
			gameVars_["lastMode"] = gameVars_["gameMode"];
			gameVars_["gameMode"] = "none";
			for (unsigned int i = 0; i < targetControllers_.Size(); i++)
			{
				targetControllers_[i]->SetCanCreateTargets(false);
				targetControllers_[i]->RemoveChilds();
			}

			float accuracy = gameStats_["shotsHit"].GetFloat() / gameStats_["shotsFired"].GetFloat();
			char s[20];
			sprintf(s, "%0.2f", accuracy * 100);
			String str = s;

			int finalScore = 0;
			if (gameStats_["points"].GetInt() > 0)
				finalScore = (int)ceil(gameStats_["points"].GetInt() * (accuracy + ((1 - accuracy) / 2)));

			resultText_->SetText(
				"Points earned: " + (String)gameStats_["points"] + "\n"
				"Accuracy: " + s + "%\n"
				"Targets destroyed: " + (String)gameStats_["targetsDestroyed"] + "\n\n"
				"FINAL SCORE: " + (String)finalScore + " !!!"
			);

			gameVars_["tempPoints"] = finalScore;
			gameStats_["shotsFired"] = 0;
			gameStats_["shotsHit"] = 0;
			gameStats_["targetsDestroyed"] = 0;
			gameStats_["targetMissed"] = 0;
			gameStats_["points"] = 0;

			resultWindow_->SetVisible(true);
			wh_->Push(resultWindow_);
			input->SetMouseVisible(true);
			weaponCrosshair_->SetVisible(false);
		}
	}

	// update UI
	pointsText_->SetText(
		"Points: " + (String)gameStats_["points"] + "\n"
		"Shots Fired: " + (String)gameStats_["shotsFired"] + "\n"
		"Targets Hit: " + (String)gameStats_["targetsDestroyed"] + "\n"
	);

	char s[20];
	sprintf(s, "%0.2f", gameVars_["timeLeft"].GetFloat());
	String str = s;

	if (gameVars_["gameMode"].GetString() == "mode_1" || gameVars_["gameMode"].GetString() == "mode_2")
	{
		timerText_->SetText(
			"Time Left: " + str + "s\n"
		);
	}
	else if (gameVars_["gameMode"].GetString() == "mode_3")
	{
		timerText_->SetText(
			"Time Elapsed: " + str + "s\n"
			"Targets: " + (String)gameStats_["m3_targetsLeft"].GetInt() + " / 18"
		);
	}
	else
	{
		timerText_->SetText("");
	}
	
}

void Weapon::CreateBullet()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Node * bulletNode = GetScene()->CreateChild("BulletNode");

	Vector3 pos = GetNode()->GetWorldPosition() + GetNode()->GetWorldRotation() * weaponsData_[gameVars_["selectedWeapon"].GetString()]["muzzlePosition"].GetVector3();

	bulletNode->SetPosition(pos);
	bulletNode->SetWorldScale(weaponsData_[gameVars_["selectedWeapon"].GetString()]["bulletScale"].GetVector3());
	bulletNode->SetWorldRotation(cameraNode_->GetWorldRotation());

	StaticModel * object = bulletNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/Bullet.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Materials/Bullet.xml"));

	// Create rigidbody, and set non-zero mass so that the body becomes dynamic
	RigidBody* body = bulletNode->CreateComponent<RigidBody>();
	body->SetMass(0.03f);
	body->SetUseGravity(false);
	body->SetTrigger(true);
	body->SetLinearVelocity((bulletNode->GetWorldRotation()) * Vector3(0, 0, 1) * 100.0f);

	shotLightNode_->SetEnabled(true);
	shotFireNode_->SetEnabled(true);
	shotFireEnabledTime_ = .0f;

	gameStats_["shotsFired"] = gameStats_["shotsFired"].GetInt() + 1;
}

void Weapon::FindHit()
{
	Vector3 hitPos;
	Drawable* hitDrawable;
	float hitDistance;

	if(Raycast(250.0f, hitPos, hitDrawable, hitDistance))
	{
		if (hitDrawable->GetNode()->GetVar("tag").ToString() == "box" || hitDrawable->GetNode()->GetVar("tag").ToString() == "human_target")
		{
			Target * target = hitDrawable->GetNode()->GetComponent<Target>();
			if (target)
				target->RegisterHit(20.0f, hitDistance);

			gameStats_["shotsHit"] = gameStats_["shotsHit"].GetInt() + 1;

			ResourceCache* cache = GetSubsystem<ResourceCache>();
			Sound* sound = cache->GetResource<Sound>("Sounds/metal.wav");

			SoundSource* soundSource = soundNode_->CreateComponent<SoundSource>();
			soundSource->Play(sound);
			soundSource->SetGain(1.0f);
		}
		else if (hitDrawable->GetNode()->GetVar("tag").ToString() == "start_button_1" && gameVars_["gameMode"].GetString() == "none")
		{
			for (unsigned int i = 0; i < targetControllers_.Size(); i++)
			{
				if (targetControllers_[i]->GetNode()->GetVar("type").GetString() == "mode_1")
				{
					targetControllers_[i]->SetCanCreateTargets(true);
				}
				else
					targetControllers_[i]->SetCanCreateTargets(false);
			}
			gameVars_["gameMode"] = "mode_1";
			gameVars_["timeLeft"] = 30.0f;

			gameStats_["shotsFired"] = 0;
			gameStats_["shotsHit"] = 0;
			gameStats_["targetsDestroyed"] = 0;
			gameStats_["targetMissed"] = 0;
			gameStats_["points"] = 0;
		}
		else if (hitDrawable->GetNode()->GetVar("tag").ToString() == "start_button_2" && gameVars_["gameMode"].GetString() == "none")
		{
			for (unsigned int i = 0; i < targetControllers_.Size(); i++)
			{
				if (targetControllers_[i]->GetNode()->GetVar("type").GetString() == "mode_1")
				{
					targetControllers_[i]->SetCanCreateTargets(true);
				}
				else
					targetControllers_[i]->SetCanCreateTargets(false);
			}

			gameVars_["gameMode"] = "mode_2";
			gameVars_["timeLeft"] = 30.0f;

			gameStats_["shotsFired"] = 0;
			gameStats_["shotsHit"] = 0;
			gameStats_["targetsDestroyed"] = 0;
			gameStats_["targetMissed"] = 0;
			gameStats_["points"] = 0;
		}
		else if (hitDrawable->GetNode()->GetVar("tag").ToString() == "start_button_4" && gameVars_["gameMode"].GetString() == "none")
		{
			gameVars_["gameMode"] = "mode_3";
			gameVars_["timeLeft"] = .0f;

			gameStats_["shotsFired"] = 0;
			gameStats_["shotsHit"] = 0;
			gameStats_["targetsDestroyed"] = 0;
			gameStats_["targetMissed"] = 0;
			gameStats_["points"] = 0;
			gameStats_["m3_targetsLeft"] = 18;

			HumanTargetController * ht_controller = GetScene()->GetComponent<HumanTargetController>();
			ht_controller->ResetTargets();
		}
		else
		{
			PaintDecal(hitPos, hitDrawable);
		}
	}
}

bool Weapon::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable, float& hitDistance)
{
	hitDrawable = 0;

	UI* ui = GetSubsystem<UI>();
	SetRandomSeed(Random(1, M_MAX_INT));
	Vector2 spread = Vector2(Random(-burstCounter_, burstCounter_), Random(-burstCounter_, 0.0f)) * weaponsData_[gameVars_["selectedWeapon"].GetString()]["spreadFactor"].GetFloat();
	IntVector2 pos = ui->GetCursorPosition();

	Graphics* graphics = GetSubsystem<Graphics>();
	Camera* camera = cameraNode_->GetComponent<Camera>();
	Ray cameraRay = camera->GetScreenRay(((float)pos.x_ + spread.x_) / graphics->GetWidth(), ((float)pos.y_ + spread.y_) / graphics->GetHeight());
	PODVector<RayQueryResult> results;
	RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
	GetScene()->GetComponent<Octree>()->RaycastSingle(query);
	if (results.Size())
	{
		RayQueryResult& result = results[0];
		hitPos = result.position_;
		hitDistance = result.distance_;
		hitDrawable = result.drawable_;
		return true;
	}

	return false;
}

void Weapon::ChangeWeapon(String weaponName)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node * weaponNode = GetNode();
	weaponNode->SetPosition(weaponsData_[weaponName]["position"].GetVector3());
	weaponNode->SetWorldScale(weaponsData_[weaponName]["scale"].GetVector3());
	weaponNode->SetRotation(weaponsData_[weaponName]["rotation"].GetQuaternion());

	StaticModel * object = weaponNode->GetComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>(weaponsData_[weaponName]["model"].GetString()));
	object->SetMaterial(cache->GetResource<Material>(weaponsData_[weaponName]["material"].GetString()));
	object->SetCastShadows(true);

	shotLightNode_->SetWorldPosition(weaponNode->GetWorldPosition() + weaponNode->GetWorldRotation() * weaponsData_[weaponName]["muzzlePosition"].GetVector3());
	shotLightNode_->SetRotation(weaponsData_[weaponName]["lightRotation"].GetQuaternion());
	shotFireNode_->SetWorldPosition(weaponNode->GetWorldPosition() + weaponNode->GetWorldRotation() * weaponsData_[weaponName]["muzzlePosition"].GetVector3());
	shotFireNode_->SetRotation(weaponsData_[weaponName]["fireRotation"].GetQuaternion());

	burstCounter_ = .0f;
}

void Weapon::PaintDecal(Vector3 hitPos, Drawable * hitDrawable)
{
	// Check if target scene node already has a DecalSet component. If not, create now
	Node* targetNode = hitDrawable->GetNode();
	if (!targetNode)
		return;

	if (targetNode->GetName() == "WeaponShotFireNode")
		return;

	DecalSet* decal = targetNode->GetComponent<DecalSet>();
	if (!decal)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();

		decal = targetNode->CreateComponent<DecalSet>();
		decal->SetMaterial(cache->GetResource<Material>("Materials/BulletHole.xml"));
	}

	if (!cameraNode_)
	{
		cameraNode_ = GetScene()->GetChild("CameraNode");
	}

	decal->AddDecal(hitDrawable, hitPos, cameraNode_->GetRotation(), 0.2f, 1.0f, 1.0f, Vector2::ZERO,
		Vector2::ONE);
}