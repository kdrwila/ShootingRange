#pragma once

#include <Urho3D/IO/Log.h>
#include "TargetController.h"
#include "Target.h"

extern Log * log_;
extern HashMap<String, VariantMap> weaponsData_;
extern VariantMap gameVars_;
extern VariantMap gameStats_;
extern Vector<TargetController*> targetControllers_;
extern Vector<Target*> humanTargets_;