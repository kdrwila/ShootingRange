#pragma once

#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

const int CTRL_PRIMARY = 1;
const int CTRL_SECONDARY = 2;

class Weapon : public LogicComponent
{
	URHO3D_OBJECT(Weapon, LogicComponent);

public:
	/// Construct.
	Weapon(Context* context);

	Controls controls_;

	static void RegisterObject(Context* context);
	virtual void Setup(Vector<Window *> * wh, Sprite * wc, Window * rw, Text * rt);
	virtual void Start();
	void FixedUpdate(float timeStep);

private:

	bool lastUpdateShoot_ = false;
	bool canShootNextBullet = false;

	float timeCounter_ = .0f;
	float lastShoot_ = .0f;
	float burstCounter_ = .0f;
	float shotFireEnabledTime_ = .0f;
	
	SharedPtr<Node> cameraNode_;
	WeakPtr<Node> shotLightNode_;
	WeakPtr<Node> shotFireNode_;

	Node * soundNode_;

	Window * resultWindow_;

	Text * pointsText_;
	Text * timerText_;
	Text * resultText_;

	Vector<Window *> * wh_;

	Sprite * weaponCrosshair_;

	void ChangeWeapon(String weaponName);
	
	void CreateBullet();
	void FindHit();
	void PaintDecal(Vector3 hitPos, Drawable * hitDrawable);

	bool Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable, float& hitDistance);
};