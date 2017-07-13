#pragma once
#include "xmfile.h"

namespace bb
{
	namespace xm
	{
		class channel
		{
		public:
			channel(document* doc, short index);

			void handleNote(const note* note);

			void release();

			void cut();

			void tick(const pattern* pat, const short row, const short tick, short* buffer, size_t length);

		private:
			document* doc;
			short index;
			
			const instrument* currentInstrument = nullptr;
			const sample*     currentSample     = nullptr;
			const note*       currentNote       = nullptr;

			float  pitch;
			float  pitchUnmodified;
			float  samplePosition;
			float  sampleAdvance;
			float  sampleFrequency;
			bool   samplePingPong;

			float  volume;
			float  panning;

			float  envelopeVolumeValue;
			size_t envelopeVolumeTick;
			float  envelopePanningValue;
			size_t envelopePanningTick;
		};

		class player
		{
		public:
			player(document* doc);

			bool tick(short** buffer, size_t* length);
		private:
			document* doc;

			int currentPattern = 0;
			int currentRow = 0;
			int currentTick = 0;

			int currentBPM;
			int currentTempo;

			std::vector<channel> channels;

			std::vector<short> buffer;
		};
	}
}