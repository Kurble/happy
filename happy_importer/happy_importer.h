#pragma once
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int fbxImporter(string fbxPath, string skinOutPath, string animOutPath);
int texImporter(string nmPath, string rmPath, string fmPath, string texOutPath);