#pragma once

#include "serialize.hpp"

namespace bb
{
	namespace xm
	{
		struct note
		{
			note()
				: compress(0)
				, pitch(0)
				, instrument(0)
				, volume(0)
				, effect(0)
				, effectParameter(0) {}

			char compress;
			char pitch;
			char instrument;
			char volume;
			char effect;
			char effectParameter;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				compress = (char)128;
				if (pitch)            compress |= 1;
				if (instrument)       compress |= 2;
				if (volume)           compress |= 4;
				if (effect)           compress |= 8;
				if (effectParameter)  compress |= 16;
				if (compress == 159) compress = 0;
				visit("compress", compress);
				if (compress & 128)
				{
					if (compress &  1) visit("pitch", pitch);
					if (compress &  2) visit("instrument", instrument);
					if (compress &  4) visit("volume", volume);
					if (compress &  8) visit("effect", effect);
					if (compress & 16) visit("effectParameter", effectParameter);
				}
				else
				{
					pitch = compress;
					visit("instrument", instrument);
					visit("volume", volume);
					visit("effect", effect);
					visit("effectParameter", effectParameter);
				}
			}
		};
		
		struct row
		{
			short channels;
			row(short channels) : channels(channels) {}

			std::vector<note> notes;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				notes.resize(channels);
				for (note& n : notes)
					n.reflect(visit);
			}
		};

		struct pattern
		{
			short channels;
			pattern(short channels) : channels(channels) {}

			int length;
			char packType;
			short rowCount;
			short dataSize;

			std::vector<row> rows;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				visit("length", length);
				visit("packType", packType);
				visit("rowCount", rowCount);
				visit("dataSize", dataSize);

				rows.resize((size_t)rowCount, row(channels));
				for (short i = 0; i < rowCount; ++i)
				{
					rows[i].reflect(visit);
				}
			}
		};

		struct sample
		{
			int   length;
			int   loopStart;
			int   loopEnd;
			char  volume;
			char  finetune;
			char  type;
			char  pan;
			char  pitch;
			char  reserved;
			char  name[22];

			std::vector<char> data;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				visit("length", length);
				visit("loopStart", loopStart);
				visit("loopEnd", loopEnd);
				visit("volume", volume);
				visit("finetune", finetune);
				visit("type", type);
				visit("pan", pan);
				visit("pitch", pitch);
				visit("reserved", reserved);
				visit.raw("name", name, 22);
				data.resize(length);
				visit.raw("data", data.data(), data.size());
			}
		};

		struct instrument
		{
			int   length;
			char  name[22];
			char  type;
			short instrumentSampleCount;
			int   instrumentSampleHeaderLength;
			char  sampleIds[96];
			char  volumeEnvelope[48];
			char  panningEnvelope[48];
			char  volumePointCount;
			char  panningPointCount;
			char  volumeSustainPoint;
			char  volumeLoopStart;
			char  volumeLoopEnd;
			char  panningSustainPoint;
			char  panningLoopStart;
			char  panningLoopEnd;
			char  volumeType;
			char  panningType;
			char  vibratoType;
			char  vibratoSweep;
			char  vibratoDepth;
			char  vibratoRate;
			short volumeFadeout;
			short reserved;

			std::vector<sample> instrumentSamples;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				visit("length", length);
				visit.raw("name", name, 22);
				visit("type", type);
				visit("instrumentSampleCount", instrumentSampleCount);

				if (instrumentSampleCount > 0)
				{
					visit("instrumentSampleHeaderLength", instrumentSampleHeaderLength);
					visit.raw("sampleIds", sampleIds, 96);
					visit.raw("volumeEnvelope", volumeEnvelope, 48);
					visit.raw("panningEnvelope", panningEnvelope, 48);
					visit("volumePointCount", volumePointCount);
					visit("panningPointCount", panningPointCount);
					visit("volumeSustainPoint", volumeSustainPoint);
					visit("volumeLoopStart", volumeLoopStart);
					visit("volumeLoopEnd", volumeLoopEnd);
					visit("panningSustainPoint", panningSustainPoint);
					visit("panningLoopStart", panningLoopStart);
					visit("panningLoopEnd", panningLoopEnd);
					visit("volumeType", volumeType);
					visit("panningType", panningType);
					visit("vibratoType", vibratoType);
					visit("vibratoSweep", vibratoSweep);
					visit("vibratoDepth", vibratoDepth);
					visit("vibratoRate", vibratoRate);
					visit("volumeFadeout", volumeFadeout);
					visit("reserved", reserved);

					int leftOver = length - 243;
					if (leftOver > 0)
					{
						vector<char> stub(leftOver);
						visit.raw("stub", stub.data(), stub.size());
					}

					instrumentSamples.resize(instrumentSampleCount);
					for (sample& s : instrumentSamples)
					{
						s.reflect(visit);
					}
				}
				else
				{
					int leftOver = length - 29;
					if (leftOver > 0)
					{
						vector<char> stub(leftOver);
						visit.raw("stub", stub.data(), stub.size());
					}
				}
			}
		};

		struct document
		{
			char  head[17];
			char  track_name[20];
			char  magic_id;
			char  tracker_name[20];
			char  tracker_major;
			char  tracker_minor;
			int   header_size;
			short length;
			short restart_position;
			short channels;
			short patternCount;
			short instrumentCount;
			short flags;
			short tempo;
			short bpm;
			char  pattern_order[256];

			std::vector<pattern> patterns;
			std::vector<instrument> instruments;

			template <typename VISITOR>
			void reflect(VISITOR& visit)
			{
				visit.raw("head", head, 17);
				visit.raw("track_name", track_name, 20);
				visit("magic_id", magic_id);
				visit.raw("tracker_name", tracker_name, 20);
				visit("tracker_major", tracker_major);
				visit("tracker_minor", tracker_minor);
				visit("header_size", header_size);
				visit("length", length);
				visit("restart_position", restart_position);
				visit("channels", channels);
				visit("patternCount", patternCount);
				visit("instrumentCount", instrumentCount);
				visit("flags", flags);
				visit("tempo", tempo);
				visit("bpm", bpm);
				visit.raw("pattern_order", pattern_order, 256);

				patterns.resize((size_t)patternCount, pattern(channels));
				for (pattern& p : patterns)
				{
					p.reflect(visit);
				}

				instruments.resize((size_t)instrumentCount);
				for (instrument& i : instruments)
				{
					i.reflect(visit);
				}
			}
		};

		class channel
		{
		public:
			channel(document* doc, short index);

			void tick(const pattern* pat, const short row, const short tick, const short* buffer, size_t length);

		private:
			document* doc;
			short index;

			// state stuff
			// todo
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
