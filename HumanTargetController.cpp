#include <cmath>
#include <string>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Window.h>

#include "Target.h"
#include "HumanTargetController.h"
#include "Global.h"

HumanTargetController::HumanTargetController(Context* context) :
	LogicComponent(context)
{
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void HumanTargetController::RegisterObject(Context* context)
{
	context->RegisterFactory<HumanTargetController>();
}

void HumanTargetController::Start()
{
	log_->Write(LOG_DEBUG, "Human Target Controller created");
	targetsLeft_ = humanTargets_;
}

void HumanTargetController::Setup(Vector<Window *> * wh, Sprite * wc, Window * rw, Text * rt)
{
	wh_ = wh;
	weaponCrosshair_ = wc;
	resultWindow_ = rw;
	resultText_ = rt;
}

void HumanTargetController::ShowTarget()
{
	if (targetsLeft_.Size() < 1 || gameVars_["gameMode"].GetString() != "mode_3" || !canShowTarget)
		return;

	if (firstTarget_)
	{
		innocentTargets_ = 2;
		firstTarget_ = false;

	}

	SetRandomSeed(Time::GetSystemTime());
	int rand = Random(0, targetsLeft_.Size());
	targetToMove = rand;
	canShowTarget = false;
	Node * node = targetsLeft_.At(rand)->GetNode();
	heightToMove = node->GetPosition().y_ + 1.5f;

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	StaticModel * model = node->GetComponent<StaticModel>();
	SetRandomSeed(Time::GetSystemTime());
	int victim = Random(0, 3);

	if (victim == 1 && innocentTargets_ > 0)
	{	
		model->SetMaterial(1, cache->GetResource<Material>("Materials/victim.xml"));
		innocentTargets_--;
		targetsLeft_.At(rand)->HT_SetVictim(true);
	}

	targetsLeft_.At(rand)->HT_SetHT(true);
	targetsLeft_.At(rand)->HT_SetActive(true);
	targetsLeft_.At(rand)->SetHealth(10.0f);

	log_->Write(LOG_DEBUG, "Human Targets Left: " + (String)targetsLeft_.Size() + " rand: " + (String)rand + " victim: " + (String)victim);
}

void HumanTargetController::FixedUpdate(float timeStep)
{
	if (gameVars_["gameMode"].GetString() != "mode_3" || targetToMove == -1)
		return;

	Node * node = targetsLeft_.At(targetToMove)->GetNode();
	log_->Write(LOG_DEBUG, "htm: " + (String)heightToMove + " vec: " + (String)node->GetPosition().ToString());
	if(node->GetPosition().y_ < heightToMove)
		node->Translate(Vector3(-.05f, .0f, .0f), TS_LOCAL);
	else
	{
		targetsLeft_.Erase(targetToMove);
		targetToMove = -1;
		log_->Write(LOG_DEBUG, "Human Target removed rand: " + (String)targetToMove);
	}
}

void HumanTargetController::SetCanShowTarget(bool toogle)
{
	canShowTarget = true;
}

void HumanTargetController::EndGame()
{
	Input* input = GetSubsystem<Input>();

	gameVars_["lastMode"] = gameVars_["gameMode"];
	gameVars_["gameMode"] = "none";

	float accuracy = gameStats_["shotsHit"].GetFloat() / gameStats_["shotsFired"].GetFloat();
	char s[20];
	sprintf(s, "%0.2f", accuracy * 100);
	String str = s;

	float finalScore = 0.0f;
	finalScore = gameVars_["timeLeft"].GetFloat() / (accuracy + ((1 - accuracy) / 2));

	sprintf(s, "%0.2f", gameVars_["timeLeft"].GetFloat());
	String str2 = s;

	sprintf(s, "%0.2f", finalScore - gameVars_["timeLeft"].GetFloat());
	String str3 = s;

	sprintf(s, "%0.2f", finalScore);
	String str4 = s;

	resultText_->SetText(
		"Time elapsed: " + str2 + "s\n"
		"Accuracy: " + str + "%\n"
		"Penalty: " + str3 + "s\n"
		"Targets destroyed: " + (String)gameStats_["targetsDestroyed"] + "\n\n"
		"FINAL SCORE: " + str4 + " !!!"
	);

	gameVars_["timeLeft"] = 0.0f;
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

void HumanTargetController::ResetTargets()
{
	targetsLeft_ = humanTargets_;
}