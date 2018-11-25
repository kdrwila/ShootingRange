#pragma once

#include "Character.h"
#include "Weapon.h"
#include "Target.h"
#include "TargetController.h"
#include "HumanTargetController.h"
#include "Destroy.h"
#include "Global.h"

const float CAMERA_MIN_DIST = 1.0f;
const float CAMERA_INITIAL_DIST = 5.0f;
const float CAMERA_MAX_DIST = 20.0f;

struct tempstruct
{
	String name;
	int points;
};

struct tempstruct_f
{
	String name;
	float points;
};

class ShootingRange : public Application
{
public:

	ShootingRange(Context * context);

	virtual void Setup();
	virtual void Start();

private:

	WeakPtr<Character> character_;
	WeakPtr<Weapon> weapon_;
	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;

	HumanTargetController * humanTargetController_;

	Sprite * weaponCrosshair_;

	Window * menuWindow_;
	Window * statisticsWindow_;
	Window * resultWindow_;

	UIElement * uiRoot_;

	Text * resultText_;
	Text * instructionText_;

	LineEdit * resultlineEdit_;
	
	Vector<Window*> * windowHierarchy_;
	StringVector * highScoreNames_[3];
	VariantVector * highScorePoints_[3];
	
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);
	void HandleClosePressed(StringHash eventType, VariantMap& eventData);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	void HandleControlClicked(StringHash eventType, VariantMap& eventData);

	void CreateScene();
	void CreateCharacter();
	void CreateWeapon();
	void CreateCrosshair();
	void CreateInstructions();
	void CreateGUI();
	void SubscribeToEvents();

	// util
	int GetModeIntFromString(String mode);
};

bool compareHighScore(const tempstruct &a, const tempstruct &b)
{
	return a.points > b.points;
}

bool compareHighScore_f(const tempstruct_f &a, const tempstruct_f &b)
{
	return a.points < b.points;
}