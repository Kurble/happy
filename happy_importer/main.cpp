#include "../stdafx.h"
#include "happy_importer.h"

int main(int argc, char** argv)
{
	if (argc >= 3)
	{
		string exec = argv[0];

		string fbx = "";
		string mesh = "";
		string skin = "";
		string anim = "";

		float scale = 1.0f;

		string nm = "";
		string rm = "";
		string fm = "";
		string texture = "";

		for (int o = 1; o < argc; o += 2)
		{
			auto val = argv[o + 1];
			string option = argv[o];

			// outputs
			if (option == "-m") mesh = val;
			if (option == "-s") skin = val;
			if (option == "-a") anim = val;
			if (option == "-t") texture = val;
			if (option == "-scale") scale = strtof(val, nullptr);

			// inputs
			if (option == "-fbx") fbx = val;
			if (option == "-nm") nm = val;
			if (option == "-rm") rm = val;
			if (option == "-fm") fm = val;
		}

		if (mesh.size() || skin.size() || anim.size())
			if (int rv = fbxImporter(fbx, mesh, skin, anim, scale)) return rv;
		if (texture.size()) 
			if (int rv = texImporter(nm, rm, fm, texture)) return rv;

		return 0;
	}
	else
	{
		cout << "usage: happy_importer \\" << endl;
		cout << "   [-fbx <input>] \\" << endl; 
		cout << "   [-m <mesh output>] \\" << endl;
		cout << "   [-s <skin output>] \\" << endl; 
		cout << "   [-a <anim output>] \\" << endl; 
		cout << "   [-nm <normal map input>] \\" << endl; 
		cout << "   [-rm <roughness map input>] \\" << endl;
		cout << "   [-fm <reflection map input>] \\" << endl;
		cout << "   [-t <multi texture output>]" << endl;

		std::string base = "C:\\Users\\re-lion\\Documents\\Code\\rts\\";

		fbxImporter(
			base + "rts_export_scripts\\mainBuilding2.FBX",
			base + "rts_resources\\Buildings\\SteamBase\\mesh.happy",
			base + "rts_resources\\Buildings\\SteamBase\\skin.happy",
			base + "rts_resources\\Buildings\\SteamBase\\idle.dance", 1.0f);

		cin.get();

		return 0;
	}
}