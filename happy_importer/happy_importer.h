#pragma once
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int fbxImporter(string fbxPath, string staticOutPath, string skinOutPath, string animOutPath, float scale);
int texImporter(string nmPath, string rmPath, string fmPath, string texOutPath);

static const uint32_t IMPORTER_VERSION = 1;
static const uint32_t MESHTYPE_STATIC = 0;
static const uint32_t MESHTYPE_SKIN = 1;