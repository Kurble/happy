#pragma once
#include "xmfile.h"

namespace bb
{
	namespace xm
	{
		class player;

		class channel
		{
		public:
			channel(document* doc, short index);

			void handleInstrument(const note* note);

			void handleNote(const note* note);

			void handleVolumeColumn(const note* note);

			void handleVolumeTick(const note* note);

			void handleEffectColumn(const note* note, player* play);

			void handleEffectTick(const note* note, player* play);

			void handleEnvelope(const envelope* env, float& val, size_t& tick);

			void handleTonePortamento();

			void handleVibrato();

			void triggerNote(bool keepPosition=false, bool keepVolume=false, bool keepPeriod=false);

			void release();

			void cut();

			void mix(float* left, float* right, size_t samples);

			void tick(const pattern* pat, player* play, const short row, const short tick, float* left, float* right, size_t samples);

		private:
			document* doc;
			short index;
			
			const instrument* currentInstrument = nullptr;
			const sample*     currentSample     = nullptr;

			unsigned char arp0 = 0;
			unsigned char arp1 = 0;

			float  pitch = 0;
			float  pitchUnmodified = 0;
			float  samplePosition = 0;
			float  sampleAdvance = 0.5f;
			float  samplePeriod = 8363;
			float  sampleFrequency = 8363;
			bool   samplePingPong = true;
			bool   sustained = false;

			float         autovibratoOffset = 0;
			float         vibratoOffset = 0;
			unsigned char vibratoSpeed = 0;
			unsigned char vibratoDepth = 0;
			unsigned char vibratoWave = 0;
			size_t        vibratoTick = 0;

			unsigned char retriggerVolume = 0;
			unsigned char retrigger = 0;

			float  portamentoPeriod = 8363;
			unsigned char portamentoParam = 0;
			unsigned char portamentoUp = 0;
			unsigned char portamentoDown = 0;

			float  fadeoutVolume = (float)0xffff;

			float  volume = 1.0f;
			float  panning = 0.5f;

			float  globalVolumeSlide = 0.0f;

			float  envelopeVolumeValue = 1.0f;
			size_t envelopeVolumeTick = 0;
			float  envelopePanningValue = 0.5f;
			size_t envelopePanningTick = 0;

		};

		class player
		{
		public:
			player(document* doc);

			void mix(short* buffer, size_t length);

			void jump(unsigned char pattern);

			void patternBreak(unsigned char row);

			void chan(int chan);

			int currentPattern = 0;
			int currentRow = 0;
			int currentTick = 0;
			int currentBPM;
			int currentTempo;
			float currentVolume = 1;

		private:
			void tick();

			document* doc;

			unsigned short jumpToPattern = 0xffff;
			unsigned short jumpToRow     = 0xffff;

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