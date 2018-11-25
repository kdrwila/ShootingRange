#pragma once

#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Window.h>

using namespace Urho3D;

class HumanTargetController : public LogicComponent
{
	URHO3D_OBJECT(HumanTargetController, LogicComponent);

public:
	/// Construct.
	HumanTargetController(Context* context);

	static void RegisterObject(Context* context);
	virtual void Start();
	virtual void Setup(Vector<Window *> * wh, Sprite * wc, Window * rw, Text * rt);

	void ShowTarget();
	void SetCanShowTarget(bool toogle);
	void ResetTargets();

	void EndGame();

	void FixedUpdate(float timeStep);

private:

	Window * resultWindow_;

	Text * resultText_;

	Vector<Window *> * wh_;

	Sprite * weaponCrosshair_;

	Vector<Target*> targetsLeft_;
	unsigned int innocentTargets_;

	int targetToMove = -1;
	float heightToMove;
	bool canShowTarget = true;
	bool moveDirection = true;
	bool firstTarget_ = true;
};