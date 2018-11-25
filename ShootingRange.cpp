#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DecalSet.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>

#include "ShootingRange.h"

URHO3D_DEFINE_APPLICATION_MAIN(ShootingRange)

using namespace Urho3D;

Log * log_;
HashMap<String, VariantMap> weaponsData_;
VariantMap gameVars_;
VariantMap gameStats_;
Vector<TargetController*> targetControllers_;
Vector<Target*> humanTargets_;

ShootingRange::ShootingRange(Context * context) : Application(context)
{
	Character::RegisterObject(context);
	Weapon::RegisterObject(context);
	Target::RegisterObject(context);
	TargetController::RegisterObject(context);
	HumanTargetController::RegisterObject(context);
	Destroy::RegisterObject(context);
}

void ShootingRange::Setup()
{
	engineParameters_["FullScreen"]			= false;
	engineParameters_["WindowWidth"]		= 0;
	engineParameters_["WindowHeight"]		= 0;
	engineParameters_["WindowResizable"]	= true;
	engineParameters_["TripleBuffer"]		= true;
	engineParameters_["Borderless"]			= true;
	engineParameters_["Multisample"]		= 4;
	engineParameters_["TextureAnisotropy"]	= 16;
	engineParameters_["MaterialQuality"]	= 4;
	engineParameters_["TextureQuality"]		= 4;

	log_ = new Log(context_);
	log_->Open("shooting_range_debug.log");
	log_->SetLevel(LOG_DEBUG);

	weaponsData_["ak47"]["position"] = Vector3(.2f, -.25f, 0.7f);
	weaponsData_["ak47"]["scale"] = Vector3(.001f, .001f, .001f);
	weaponsData_["ak47"]["rotation"] = Quaternion(90, Vector3(0, 0, 1));
	weaponsData_["ak47"]["model"] = "Models/AK.mdl";
	weaponsData_["ak47"]["material"] = "Materials/AK.xml";
	weaponsData_["ak47"]["sound"] = "Sounds/AK.wav";
	weaponsData_["ak47"]["shootingInterval"] = 0.100f;
	weaponsData_["ak47"]["maxSpreadTime"] = 1.5f;
	weaponsData_["ak47"]["spreadFactor"] = 75.0f;
	weaponsData_["ak47"]["automatic"] = true;
	weaponsData_["ak47"]["muzzlePosition"] = Vector3(.13f, .02f, .2f);
	weaponsData_["ak47"]["bulletScale"] = Vector3(.0042f, .0042f, .0042f);
	weaponsData_["ak47"]["lightRotation"] = Quaternion(.0f, .0f, .0f);
	weaponsData_["ak47"]["fireRotation"] = Quaternion(-90.0f, .0f, .0f);

	weaponsData_["glock"]["position"] = Vector3(.34f, -.27f, 0.7f);
	weaponsData_["glock"]["scale"] = Vector3(.0013f, .0013f, .0013f);
	weaponsData_["glock"]["rotation"] = Quaternion(90, Vector3(0, -1, 0));
	weaponsData_["glock"]["model"] = "Models/Glock.mdl";
	weaponsData_["glock"]["material"] = "Materials/Glock.xml";
	weaponsData_["glock"]["sound"] = "Sounds/Glock.wav";
	weaponsData_["glock"]["shootingInterval"] = 0.15f;
	weaponsData_["glock"]["maxSpreadTime"] = 1.5f;
	weaponsData_["glock"]["spreadFactor"] = 75.0f;
	weaponsData_["glock"]["automatic"] = false;
	weaponsData_["glock"]["muzzlePosition"] = Vector3(.16f, .08f, .0f);
	weaponsData_["glock"]["bulletScale"] = Vector3(.0042f, .0042f, .0042f);
	weaponsData_["glock"]["lightRotation"] = Quaternion(.0f, 90.0f, .0f);
	weaponsData_["glock"]["fireRotation"] = Quaternion(.0f, .0f, 90.0f);

	gameVars_["selectedWeapon"] = "ak47";
	gameVars_["primaryWeapon"] = "ak47";
	gameVars_["secondaryWeapon"] = "glock";
	gameVars_["gameMode"] = "none";
	gameVars_["timeLeft"] = 0.0f;
	gameVars_["tempPoints"] = 0;
	gameVars_["lastMode"] = "none";

	gameStats_["shotsFired"] = 0;
	gameStats_["shotsHit"] = 0;
	gameStats_["targetsDestroyed"] = 0;
	gameStats_["targetMissed"] = 0;
	gameStats_["points"] = 0;

	for (unsigned int i = 0; i < 3; i++)
	{
		highScoreNames_[i] = new StringVector();
		highScorePoints_[i] = new VariantVector();
	}

	windowHierarchy_ = new Vector<Window *>();
}

void ShootingRange::Start()
{
	// create static scene content
	CreateScene();

	// create the controllable character
	CreateCharacter();

	// create the UI content
	CreateInstructions();
	CreateGUI();

	// create weapon
	CreateCrosshair();
	CreateWeapon();

	// subscribe to necessary events
	SubscribeToEvents();

	// read saved high scores.
	File file(GetContext());
	for (unsigned int i = 0; i < 3; i++)
	{
		if (file.Open("Data/Saved/highscore_names_" + (String)i + ".srsf", FILE_READ))
		{
			highScoreNames_[i] = new StringVector(file.ReadStringVector());
			file.Close();
		}

		if (file.Open("Data/Saved/highscore_points_" + (String)i + ".srsf", FILE_READ))
		{
			highScorePoints_[i] = new VariantVector(file.ReadVariantVector());
			file.Close();
		}
	}
}

void ShootingRange::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;
	int key = eventData[P_KEY].GetInt();
	if (key == KEY_ESCAPE)
	{
		Input * input = GetSubsystem<Input>();

		if (windowHierarchy_->Size() <= 0)
		{
			menuWindow_->SetVisible(true);
			input->SetMouseVisible(true);
			weaponCrosshair_->SetVisible(false);
			windowHierarchy_->Push(menuWindow_);
		}
		else
		{
			windowHierarchy_->Back()->SetVisible(false);
			windowHierarchy_->Pop();

			if (windowHierarchy_->Size() <= 0)
			{
				input->SetMouseVisible(false);
				weaponCrosshair_->SetVisible(true);
			}
			else
				windowHierarchy_->Back()->SetVisible(true);
		}
	}
}

void ShootingRange::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
	windowHierarchy_->Back()->SetVisible(false);
	windowHierarchy_->Pop();

	if (windowHierarchy_->Size() <= 0)
	{
		GetSubsystem<Input>()->SetMouseVisible(false);
		weaponCrosshair_->SetVisible(true);
	}
	else
		windowHierarchy_->Back()->SetVisible(true);
}

void ShootingRange::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);

	File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/test_scene.xml", FILE_READ);
	scene_->LoadXML(loadFile);

	// get all target controllers
	PODVector<Node *> childs;
	scene_->GetChildrenWithComponent<TargetController>(childs, true);

	for (unsigned int i = 0; i < childs.Size(); i++)
	{
		if (i % 2 == 1)
		{
			targetControllers_.Back()->AddPairedController(childs[i]->GetComponent<TargetController>());
			continue;
		}

		targetControllers_.Push(childs[i]->GetComponent<TargetController>());
	}

	// get all human targets
	scene_->GetChildrenWithTag(childs, "human_target", true);

	for (unsigned int i = 0; i < childs.Size(); i++)
	{
		childs[i]->Translate(Vector3(1.5f, 0.0f, 0.0f));
		Node *  node = childs[i]->GetChild("Points");
		node->SetEnabled(false);
		humanTargets_.Push(childs[i]->GetComponent<Target>());
	}

	humanTargetController_ = scene_->CreateComponent<HumanTargetController>();

	cameraNode_ = scene_->CreateChild("CameraNode");
	Camera* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetFarClip(300.0f);
	camera->SetFov(70.0f);

	Renderer* renderer = GetSubsystem<Renderer>();

	renderer->SetHDRRendering(true);

	// Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
	renderer->SetViewport(0, viewport);

	// Add post-processing effects appropriate with the example scene
	SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
	effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA3.xml"));
	effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/ColorCorrection.xml"));

	viewport->SetRenderPath(effectRenderPath);
}

void ShootingRange::CreateCharacter()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node * spawn = scene_->GetChild("Spawn");

	Node* objectNode = spawn->CreateChild("Jack");
	objectNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

	// spin node
	Node* adjustNode = objectNode->CreateChild("AdjNode");
	adjustNode->SetRotation(Quaternion(180, Vector3(0, 1, 0)));

	// Create the rendering component + animation controller
	AnimatedModel* object = adjustNode->CreateComponent<AnimatedModel>();
	object->SetModel(cache->GetResource<Model>("Models/Mutant/Mutant.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
	object->SetCastShadows(true);
	adjustNode->CreateComponent<AnimationController>();

	// Set the head bone for manual control
	object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;

	// Create rigidbody, and set non-zero mass so that the body becomes dynamic
	RigidBody* body = objectNode->CreateComponent<RigidBody>();
	body->SetCollisionLayer(1);
	body->SetMass(1.0f);

	// Set zero angular factor so that physics doesn't turn the character on its own.
	// Instead we will control the character yaw manually
	body->SetAngularFactor(Vector3::ZERO);

	// Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
	body->SetCollisionEventMode(COLLISION_ALWAYS);

	// Set a capsule shape for collision
	CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
	shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.7f));

	// Create the character logic component, which takes care of steering the rigidbody
	// Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
	// and keeps it alive as long as it's not removed from the hierarchy
	character_ = objectNode->CreateComponent<Character>();
}

void ShootingRange::CreateWeapon()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node * weaponNode = cameraNode_->CreateChild("WeaponNode");
	weaponNode->SetPosition(weaponsData_[gameVars_["selectedWeapon"].GetString()]["position"].GetVector3());
	weaponNode->SetWorldScale(weaponsData_[gameVars_["selectedWeapon"].GetString()]["scale"].GetVector3());
	weaponNode->SetRotation(weaponsData_[gameVars_["selectedWeapon"].GetString()]["rotation"].GetQuaternion());

	StaticModel * object = weaponNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>(weaponsData_[gameVars_["selectedWeapon"].GetString()]["model"].GetString()));
	object->SetMaterial(cache->GetResource<Material>(weaponsData_[gameVars_["selectedWeapon"].GetString()]["material"].GetString()));
	object->SetCastShadows(true);

	Node* lightNode = weaponNode->CreateChild("WeaponShotLightNode");
	lightNode->SetWorldPosition(weaponNode->GetWorldPosition() + weaponNode->GetWorldRotation() * weaponsData_[gameVars_["selectedWeapon"].GetString()]["muzzlePosition"].GetVector3());
	lightNode->SetRotation(weaponsData_[gameVars_["selectedWeapon"].GetString()]["lightRotation"].GetQuaternion());
	lightNode->SetEnabled(false);
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_SPOT);
	light->SetColor(Color(1.0f, .96f, .72f));
	light->SetRange(50.0f);
	light->SetFov(270.0f);
	light->SetCastShadows(true);

	Node * fireNode = weaponNode->CreateChild("WeaponShotFireNode");
	fireNode->SetWorldPosition(weaponNode->GetWorldPosition() + weaponNode->GetWorldRotation() * weaponsData_[gameVars_["selectedWeapon"].GetString()]["muzzlePosition"].GetVector3());
	fireNode->SetWorldScale(Vector3(.25f, .25f, .25f));
	fireNode->SetRotation(weaponsData_[gameVars_["selectedWeapon"].GetString()]["fireRotation"].GetQuaternion());
	fireNode->SetEnabled(false);
	object = fireNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Materials/GunFire.xml"));
	object->SetCastShadows(false);

	weapon_ = weaponNode->CreateComponent<Weapon>();
	weapon_->Setup(windowHierarchy_, weaponCrosshair_, resultWindow_, resultText_);
	humanTargetController_->Setup(windowHierarchy_, weaponCrosshair_, resultWindow_, resultText_);
}

void ShootingRange::CreateCrosshair()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	Graphics* graphics = GetSubsystem<Graphics>();
	float width = (float)graphics->GetWidth();
	float height = (float)graphics->GetHeight();

	weaponCrosshair_ = ui->GetRoot()->CreateChild<Sprite>();
	Texture2D* texture_ch = cache->GetResource<Texture2D>("Textures/ch.png");
	weaponCrosshair_->SetTexture(texture_ch);
	weaponCrosshair_->SetSize(350, 350);
	weaponCrosshair_->SetHotSpot(175, 175);
	weaponCrosshair_->SetPosition((width / 2), (height / 2));
	weaponCrosshair_->SetBlendMode(BlendMode::BLEND_ALPHA);
	weaponCrosshair_->SetColor(Color(1.0f, 0.0f, 0.0f));
	weaponCrosshair_->SetVisible(true);
}

void ShootingRange::CreateInstructions()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	uiRoot_ = ui->GetRoot();

	// Construct new Text object, set string to display and font to use
	instructionText_ = ui->GetRoot()->CreateChild<Text>();
	instructionText_->SetText(
		"Use WASD keys and mouse to move\n"
		"Space to jump\n"
		"Shift to sprint\n"
		"R to respawn\n"
		"Key 1, 2 to change weapons\n"
		"F1 to show/hide instructions\n"
	);
	instructionText_->SetFont(cache->GetResource<Font>("Fonts/Prototype.ttf"), 20);
	// The text has multiple rows. Center them in relation to each other
	instructionText_->SetTextAlignment(HA_CENTER);

	// Position the text relative to the screen center
	instructionText_->SetHorizontalAlignment(HA_CENTER);
	instructionText_->SetVerticalAlignment(VA_CENTER);
	instructionText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void ShootingRange::CreateGUI()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	uiRoot_->SetDefaultStyle(style);

	// menu window
	menuWindow_ = new Window(context_);
	uiRoot_->AddChild(menuWindow_);
	menuWindow_->SetMinWidth(250);
	menuWindow_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	menuWindow_->SetAlignment(HA_CENTER, VA_CENTER);
	menuWindow_->SetName("Window");
	menuWindow_->SetVisible(false);

	UIElement* titleBar = new UIElement(context_);
	titleBar->SetMinSize(0, 24);
	titleBar->SetVerticalAlignment(VA_TOP);
	titleBar->SetLayoutMode(LM_HORIZONTAL);

	Button* buttonClose = new Button(context_);
	buttonClose->SetName("CloseButton");

	titleBar->AddChild(buttonClose);
	menuWindow_->AddChild(titleBar);

	menuWindow_->SetStyleAuto();
	buttonClose->SetStyle("CloseButton");

	SubscribeToEvent(buttonClose, E_RELEASED, URHO3D_HANDLER(ShootingRange, HandleClosePressed));
	SubscribeToEvent(E_UIMOUSECLICK, URHO3D_HANDLER(ShootingRange, HandleControlClicked));

	Button* button = new Button(context_);
	button->SetName("resume");
	button->SetMinHeight(24);
	Text* text = button->CreateChild<Text>();
	text->SetText("Resume");
	text->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	text->SetTextAlignment(HA_CENTER);
	text->SetAlignment(HA_CENTER, VA_CENTER);
	menuWindow_->AddChild(button);
	button->SetStyleAuto();

	button = new Button(context_);
	button->SetName("statistics");
	button->SetMinHeight(24);
	text = button->CreateChild<Text>();
	text->SetText("High scores");
	text->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	text->SetTextAlignment(HA_CENTER);
	text->SetAlignment(HA_CENTER, VA_CENTER);
	menuWindow_->AddChild(button);
	button->SetStyleAuto();

	button = new Button(context_);
	button->SetName("exitGame");
	button->SetMinHeight(24);
	text = button->CreateChild<Text>();
	text->SetText("Exit game");
	text->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	text->SetTextAlignment(HA_CENTER);
	text->SetAlignment(HA_CENTER, VA_CENTER);
	menuWindow_->AddChild(button);
	button->SetStyleAuto();

	// profiles window
	statisticsWindow_ = new Window(context_);
	uiRoot_->AddChild(statisticsWindow_);
	statisticsWindow_->SetMinWidth(300);
	statisticsWindow_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	statisticsWindow_->SetAlignment(HA_CENTER, VA_CENTER);
	statisticsWindow_->SetName("Window");
	statisticsWindow_->SetVisible(false);

	titleBar = new UIElement(context_);
	titleBar->SetMinSize(0, 24);
	titleBar->SetVerticalAlignment(VA_TOP);
	titleBar->SetLayoutMode(LM_HORIZONTAL);

	buttonClose = new Button(context_);
	buttonClose->SetName("CloseButton");

	titleBar->AddChild(buttonClose);
	statisticsWindow_->AddChild(titleBar);

	statisticsWindow_->SetStyleAuto();
	buttonClose->SetStyle("CloseButton");

	UIElement * element = statisticsWindow_->CreateChild<UIElement>();
	element->SetName("content");
	element->SetLayout(LM_HORIZONTAL);

	SubscribeToEvent(buttonClose, E_RELEASED, URHO3D_HANDLER(ShootingRange, HandleClosePressed));

	// results window
	resultWindow_ = uiRoot_->CreateChild<Window>();
	resultWindow_->SetStyleAuto();
	resultWindow_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	resultWindow_->SetAlignment(HA_CENTER, VA_CENTER);
	resultWindow_->SetVisible(false);
	resultWindow_->SetMinWidth(300);

	resultText_ = resultWindow_->CreateChild<Text>();
	resultText_->SetText(
		""
	);
	resultText_->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	resultText_->SetTextAlignment(HA_CENTER);
	resultText_->SetPosition(10, 10);

	text = resultWindow_->CreateChild<Text>();
	text->SetText("Enter name: ");
	text->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	text->SetTextAlignment(HA_LEFT);
	text->SetPosition(10, 30);

	resultlineEdit_ = resultWindow_->CreateChild<LineEdit>();
	resultlineEdit_->SetStyleAuto();
	resultlineEdit_->SetMinHeight(16);
	resultlineEdit_->SetMaxHeight(16);

	button = resultWindow_->CreateChild<Button>();
	button->SetName("saveResults");
	button->SetStyleAuto();
	button->SetMinHeight(24);
	button->SetHorizontalAlignment(HA_CENTER);
	button->SetMaxHeight(24);
	button->SetMaxWidth(100);
	text = button->CreateChild<Text>();
	text->SetText("Save result");
	text->SetFont(cache->GetResource<Font>("Fonts/Lato-Regular.ttf"), 11);
	text->SetAlignment(HA_CENTER, VA_CENTER);
}

void ShootingRange::SubscribeToEvents()
{
	// Subscribe to Update event for setting the character controls before physics simulation
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ShootingRange, HandleUpdate));

	// Subscribe to PostUpdate event for updating the camera position after physics simulation
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(ShootingRange, HandlePostUpdate));

	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(ShootingRange, HandleKeyDown));

	// Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
	UnsubscribeFromEvent(E_SCENEUPDATE);
}

void ShootingRange::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	Input* input = GetSubsystem<Input>();
	UI* ui = GetSubsystem<UI>();

	// debug
	if (!GetSubsystem<Input>()->IsMouseVisible())
	{
		if (input->GetKeyDown(KEY_R))
		{
			character_->GetNode()->Remove();
			CreateCharacter();
			//character_->GetNode()->SetPosition(Vector3(0.0f, 2.0f, 0.0f));
		}

		if (input->GetKeyDown(KEY_F1))
		{
			instructionText_->SetVisible(!instructionText_->IsVisible());
		}
	}
	
	if (character_ && !GetSubsystem<Input>()->IsMouseVisible())
	{
		// Clear previous controls
		character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP | CTRL_SPRINT, false);

		// Update controls using keys
		if (!ui->GetFocusElement())
		{
			character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
			character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
			character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
			character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
			character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
			character_->controls_.Set(CTRL_SPRINT, input->GetKeyDown(KEY_SHIFT));

			character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
			character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
			character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
			character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));
		}
	}

	if (weapon_ && !GetSubsystem<Input>()->IsMouseVisible())
	{
		weapon_->controls_.Set(CTRL_PRIMARY | CTRL_SECONDARY, false);

		if (!ui->GetFocusElement())
		{
			weapon_->controls_.Set(CTRL_PRIMARY, input->GetKeyDown(KEY_1));
			weapon_->controls_.Set(CTRL_SECONDARY, input->GetKeyDown(KEY_2));
		}
	}

	for (unsigned int i = 0; i < targetControllers_.Size(); i++)
	{
		targetControllers_[i]->SpawnTarget();
	}

	humanTargetController_->ShowTarget();
}

void ShootingRange::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!character_)
		return;

	Node* characterNode = character_->GetNode();

	// Get camera lookat dir from character yaw + pitch
	Quaternion rot = characterNode->GetRotation();
	Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

	// Turn head to camera pitch, but limit to avoid unnatural animation
	Node* headNode = characterNode->GetChild("Mutant:Head", true);
	float limitPitch = Clamp(character_->controls_.pitch_, -45.0f, 45.0f);
	Quaternion headDir = rot * Quaternion(limitPitch, Vector3(1.0f, 0.0f, 0.0f));
	// This could be expanded to look at an arbitrary target, now just look at a point in front
	Vector3 headWorldTarget = headNode->GetWorldPosition() + headDir * Vector3(0.0f, 0.0f, -1.0f);
	headNode->LookAt(headWorldTarget, Vector3(0.0f, 1.0f, 0.0f));

	cameraNode_->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.2f));
	cameraNode_->SetRotation(dir);
}

void ShootingRange::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
	UIElement* clicked = (UIElement*)(eventData[UIMouseClick::P_ELEMENT].GetPtr());
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	if (!clicked)
		return;

	String name = clicked->GetName();
	if (name == "exitGame")
	{
		log_->Close();
		engine_->Exit();
	}
	else if (name == "resume")
	{
		menuWindow_->SetVisible(false);
		GetSubsystem<Input>()->SetMouseVisible(false);
		weaponCrosshair_->SetVisible(true);

		windowHierarchy_->Pop();
	}
	else if (name == "statistics")
	{
		statisticsWindow_->SetVisible(true);

		windowHierarchy_->Back()->SetVisible(false);
		windowHierarchy_->Push(statisticsWindow_);

		statisticsWindow_->GetChild("content", false)->RemoveAllChildren();

		using namespace std;

		for (unsigned int i = 0; i < 3; i++)
		{
			vector<tempstruct> tempvec;
			vector<tempstruct_f> tempvec_f;

			if (i == 2)
			{
				for (unsigned int x = 0; x < highScoreNames_[i]->Size(); x++)
				{
					tempstruct_f user;
					user.name = highScoreNames_[i]->At(x);
					user.points = highScorePoints_[i]->At(x).GetFloat();

					tempvec_f.push_back(user);
				}

				sort(tempvec_f.begin(), tempvec_f.end(), compareHighScore_f);
			}
			else
			{
				for (unsigned int x = 0; x < highScoreNames_[i]->Size(); x++)
				{
					tempstruct user;
					user.name = highScoreNames_[i]->At(x);
					user.points = highScorePoints_[i]->At(x).GetInt();

					tempvec.push_back(user);
				}

				sort(tempvec.begin(), tempvec.end(), compareHighScore);
			}

			UIElement * element = statisticsWindow_->GetChild("content", false)->CreateChild<UIElement>();
			element->SetLayout(LM_VERTICAL);

			int size = 0;
			if (i == 2)
				size = tempvec_f.size();
			else
				size = tempvec.size();

			for (int x = 0; x < size && x < 20; x++)
			{
				Text * text = element->CreateChild<Text>();
				if (i == 2)
					text->SetText((String)tempvec_f.at(x).name);
				else
					text->SetText((String)tempvec.at(x).name);
				text->SetFont(cache->GetResource<Font>("Fonts/Prototype.ttf"), 15);
				text->SetTextAlignment(HA_LEFT);
				text->SetAlignment(HA_CENTER, VA_CENTER);
			}

			element = statisticsWindow_->GetChild("content", false)->CreateChild<UIElement>();
			element->SetLayout(LM_VERTICAL);
			element->SetMinWidth(40);

			element = statisticsWindow_->GetChild("content", false)->CreateChild<UIElement>();
			element->SetLayout(LM_VERTICAL);

			for (int x = 0; x < size && x < 20; x++)
			{
				Text * text = element->CreateChild<Text>();
				if (i == 2)
				{
					char s[20];
					sprintf(s, "%0.2fs", tempvec_f.at(x).points);
					String str = s;

					text->SetText(str);
				}
				else
					text->SetText((String)tempvec.at(x).points);
				text->SetFont(cache->GetResource<Font>("Fonts/Prototype.ttf"), 15);
				text->SetTextAlignment(HA_RIGHT);
				text->SetAlignment(HA_CENTER, VA_CENTER);
			}

			if (i != 2)
			{
				element = statisticsWindow_->GetChild("content", false)->CreateChild<UIElement>();
				element->SetLayout(LM_VERTICAL);
				element->SetMinWidth(40);
			}
		}
	}
	else if (name == "saveResults")
	{
		if (gameVars_["tempPoints"].GetInt() < 1)
			return;

		int mode = GetModeIntFromString(gameVars_["lastMode"].GetString());

		String name = resultlineEdit_->GetText();
		name = name.Substring(0, 9);

		highScoreNames_[mode]->Push(name);
		highScorePoints_[mode]->Push(gameVars_["tempPoints"]);

		// save files
		File file(GetContext());
		file.Open("Data/Saved/highscore_names_" + (String)mode + ".srsf", FILE_WRITE);
		file.WriteStringVector(*highScoreNames_[mode]);
		file.Close();

		file.Open("Data/Saved/highscore_points_" + (String)mode + ".srsf", FILE_WRITE);
		file.WriteVariantVector(*highScorePoints_[mode]);
		file.Close();

		windowHierarchy_->Back()->SetVisible(false);
		windowHierarchy_->Pop();

		GetSubsystem<Input>()->SetMouseVisible(false);
		weaponCrosshair_->SetVisible(true);
	}
}

int ShootingRange::GetModeIntFromString(String mode)
{
	if (mode == "mode_1") return 0;
	if (mode == "mode_2") return 1;
	if (mode == "mode_3") return 2;

	return 0;
}