// v LinenTest.h
#pragma once
#include "Engine/Scripting/Script.h"

API_CLASS() class LINENFLAX_API LinenTest : public Script
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE(LinenTest);

    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};
// ^ LinenTest.h