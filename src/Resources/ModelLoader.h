#pragma once
#include "Mesh.h"
#include <string>

class ModelLoader
{
public:
    static Mesh Load(const std::string& path);
};
