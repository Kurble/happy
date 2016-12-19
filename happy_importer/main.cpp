#include "../stdafx.h"
#include "happy_importer.h"

int main(int argc, char** argv)
{
	if (argc >= 3)
	{
		string exec = argv[0];

		string fbx = "";
		string skin = "";
		string anim = "";

		string nm = "";
		string rm = "";
		string fm = "";
		string texture = "";

		for (int o = 1; o < argc; o += 2)
		{
			auto val = argv[o + 1];
			string option = argv[o];

			// outputs
			if (option == "-s") skin = val;
			if (option == "-a") anim = val;
			if (option == "-t") texture = val;

			// inputs
			if (option == "-fbx") fbx = val;
			if (option == "-nm") nm = val;
			if (option == "-rm") rm = val;
			if (option == "-fm") fm = val;
		}

		if (skin.size() || anim.size())
			if (int rv = fbxImporter(fbx, skin, anim)) return rv;
		if (texture.size()) 
			if (int rv = texImporter(nm, rm, fm, texture)) return rv;

		return 0;
	}
	else
	{
		cout << "usage: happy_importer \\" << endl;
		cout << "   [-fbx <input>] \\" << endl; 
		cout << "   [-s <skin output>] \\" << endl; 
		cout << "   [-a <anim output>] \\" << endl; 
		cout << "   [-nm <normal map input>] \\" << endl; 
		cout << "   [-rm <roughness map input>] \\" << endl;
		cout << "   [-fm <reflection map input>] \\" << endl;
		cout << "   [-t <multi texture output>]" << endl;

		std::string base = "C:\\Users\\re-lion\\Documents\\Code\\rts\\";

		fbxImporter(
			base + "max\\Buildings\\MainBuilding\\export\\Main_Building_Investor.FBX",
			base + "rts_resources\\Buildings\\SteamBase\\skin.skin",
			base + "rts_resources\\Buildings\\SteamBase\\idle.anim");

		cin.get();

		return 0;
	}
}