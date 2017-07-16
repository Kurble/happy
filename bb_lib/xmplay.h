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

			void handleInstrument(const note* note);

			void handleNote(const note* note);

			void handleVolumeColumn(const note* note);

			void handleVolumeTick(const note* note);

			void handleEffectColumn(const note* note);

			void handleEffectTick(const note* note);

			void handleEnvelope(const envelope* env, float& val, size_t& tick);

			void handleTonePortamento();

			void triggerNote(bool keepPosition=false, bool keepVolume=false, bool keepPeriod=false);

			void release();

			void cut();

			void mix(float* left, float* right, size_t samples);

			void tick(const pattern* pat, const short row, const short tick, float* left, float* right, size_t samples);

		private:
			document* doc;
			short index;
			
			const instrument* currentInstrument = nullptr;
			const sample*     currentSample     = nullptr;

			float  pitch;
			float  pitchUnmodified;
			float  samplePosition = 0;
			float  sampleAdvance = 0.5f;
			float  samplePeriod = 8363;
			float  sampleFrequency;
			bool   samplePingPong;

			float  portamentoPeriod;
			unsigned char portamentoParam;

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

			void mix(short* buffer, size_t length);

			void chan(int chan);

		private:
			void tick();

			document* doc;

			int currentPattern = 0;
			int currentRow = 0;
			int currentTick = 0;

			int currentBPM;
			int currentTempo;

			int currentChannel = -1;

			std::vector<channel> channels;
			std::vector<float>   left;
			std::vector<float>   right;
			float*               gleft;
			float*               gright;

			size_t samplesLeft = 0;
		};
	}
}