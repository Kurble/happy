#include <cassert>
#include "xmplay.h"


#define HAS_TONE_PORTAMENTO(s) ()

namespace bb
{
	namespace xm
	{
		inline bool hasTonePortamento(const note* note) { return note->effect == 3 || note->effect == 5 || (note->volume & 0xf0) == 0xF0; }

		channel::channel(document* doc, short index)
			: doc(doc)
			, index(index) { }

		void channel::handleInstrument(const note* note)
		{
			if (note->instrument)
			{
				if (note->instrument <= doc->instrumentCount)
				{
					currentInstrument = &doc->instruments[note->instrument - 1];
					if (note->pitch == 0 && currentSample)
					{
						triggerNote(true, false);
					}
				}
				else
				{
					cut();
					currentInstrument = nullptr;
					currentSample = nullptr;
				}
			}
		}

		void channel::handleNote(const note* note)
		{
			if (note->pitch > 0 && note->pitch < 97)
			{
				if (currentInstrument && currentSample && hasTonePortamento(note))
				{
					pitchUnmodified = note->pitch + currentSample->pitch + currentSample->finetune / 128.0f - 1;
					pitch = pitchUnmodified;

					if (note->instrument > 0)
						triggerNote(false, false, true);
					else
						triggerNote(false, true, true);

					portamentoPeriod = 10 * 12 * 16 * 4 - (float)(pitch) * 16 * 4;
				}
				else if (currentInstrument == nullptr || currentInstrument->instrumentSampleCount == 0)
				{
					cut();// bad instrument
				}
				else if (currentInstrument->sampleIds[note->pitch - 1] < currentInstrument->instrumentSampleCount)
				{
					currentSample = &currentInstrument->instrumentSamples[currentInstrument->sampleIds[note->pitch - 1]];
					
					pitchUnmodified = note->pitch + currentSample->pitch + currentSample->finetune / 128.0f - 1;
					pitch = pitchUnmodified;

					if (note->instrument > 0)
						triggerNote(false, false);
					else
						triggerNote(false, true);
				}
				else
				{
					cut(); // bad sample
				}
			}
			else if (note->pitch == 97)
			{
				release();
			}

			// handle volume
			//switch (currentNote->volume)
			//{
			//default:
			//	break;
			//}

			// handle effects
			//switch (currentNote->effect)
			//{
			//default:
			//	break;
			//}
		}

		void channel::handleVolumeColumn(const note* note)
		{
			switch (note->volume & 0xf0)
			{
			case 0x00: // do nothing
				break;
			case 0x50: // set volume
				if (note->volume > 0x50) break;
			case 0x10:
			case 0x20:
			case 0x30:
			case 0x40:
				volume = (note->volume - 0x10) / (float)0x40;
				break;
			case 0x80: // fine volume slide down
				volume -= (note->volume & 0x0f) / (float)0x40;
				break;
			case 0x90: // fine volume slide up
				volume += (note->volume & 0x0f) / (float)0x40;
				break;
			case 0xf0: // tone portamento
				if (note->volume & 0x0f)
				{
					portamentoParam = (note->volume & 0x0f) | ((note->volume & 0x0f) << 4);
				}
				break;
			default: // don't know
				break;
			}
		}

		void channel::handleVolumeTick(const note* note)
		{
			switch (note->volume & 0xf0)
			{
			case 0x60: // volume slide down
				volume -= (note->volume & 0x0f) / (float)0x40;
				break;
			case 0x70: // volume slide up
				volume += (note->volume & 0x0f) / (float)0x40;
				break;
			case 0xf0: // tone portamento
				handleTonePortamento();
				break;
			}
		}

		void channel::handleEffectColumn(const note* note)
		{
			switch (note->effect)
			{
			case 3:
				if (note->effectParameter)
					portamentoParam = note->effectParameter;
				break;
			}
		}

		void channel::handleEffectTick(const note* note)
		{
			switch (note->effect)
			{
			case 5:
				// volume slide + tone portamento
			case 3:
				// tone portamento
				handleTonePortamento();
				break;

			default:
				break;
			}
		}
		
		void channel::handleTonePortamento()
		{
			if (portamentoPeriod && samplePeriod != portamentoPeriod)
			{
				if (samplePeriod < portamentoPeriod)
				{
					samplePeriod = fminf(portamentoPeriod, samplePeriod + 4 * portamentoParam);
				}
				else
				{
					samplePeriod = fminf(portamentoPeriod, samplePeriod - 4 * portamentoParam);
				}
			}
		}

		void channel::triggerNote(bool keepPosition, bool keepVolume, bool keepPeriod)
		{
			if (!keepPosition)
			{
				samplePosition = 0;
				samplePingPong = true;
			}

			if (currentSample)
			{
				if (!keepVolume)
					volume = currentSample->volume / 64.0f;

				panning = currentSample->pan / 255.0f;
			}

			envelopeVolumeTick = 0;
			envelopePanningTick = 0;

			if (!keepPeriod)
			{
				samplePeriod = 10 * 12 * 16 * 4 - (float)(pitch) * 16 * 4;
			}
		}

		void channel::cut() 
		{
			volume = 0;
		}

		void channel::release() 
		{
			pitch = 97;
		}

		void channel::handleEnvelope(const envelope* env, float& val, size_t& tick)
		{
			if (env->type & 0x01) // 1 flag: env is enable
			{
				if (env->type & 0x02) // 2 flag: looping
				{
					if (tick >= env->loopEnd)
						tick -= (env->loopEnd - env->loopStart);
				}

				size_t i = 0;
				for (i = 0; i < env->pointCount - 2; ++i)
				{
					if (env->points[i].frame <= tick &&
						env->points[i + 1].frame >= tick)
					{
						break;
					}
				}

				if (tick <= env->points[i].frame)
				{
					val = env->points[i].value / 64.0f;
				}
				else if (tick >= env->points[i + 1].frame)
				{
					val = env->points[i + 1].value / 64.0f;
				}
				else
				{
					float p = (float)(tick - env->points[i].frame) / (float)(env->points[i+1].frame - env->points[i].frame);
					val = (env->points[i].value * (1 - p) + env->points[i+1].value * p) / 64.0f;
				}

				tick++;
			}
			else
			{
				val = 1.0f;
			}
		}

		void channel::mix(float* left, float* right, size_t samples)
		{
			if (currentSample && samplePosition >= 0 && pitch < 97)
			{
				//float FinalVol = (FadeOutVol / 65536)*(EnvelopeVol / 64)*(GlobalVol / 64)*(Vol / 64)*Scale;
				float finalVolume = envelopeVolumeValue * volume;
				float volLeft = (1 - panning)*finalVolume;
				float volRight = (panning)*finalVolume;

				for (size_t i = 0; i < samples; ++i)
				{
					left[i] += currentSample->samples[(size_t)samplePosition] * volLeft;
					right[i] += currentSample->samples[(size_t)samplePosition] * volRight;


					switch (currentSample->type & 0x3)
					{
					case 0: // no loop
						samplePosition += sampleAdvance;
						if (samplePosition >= currentSample->length)
							samplePosition = -1;
						break;

					case 1: // forward loop
						samplePosition += sampleAdvance;
						while (samplePosition > currentSample->loopEnd)
							samplePosition -= (currentSample->loopEnd - currentSample->loopStart);
						break;

					case 2: // pingpong loop
						if (samplePingPong)
						{
							samplePosition += sampleAdvance;

							if (samplePosition >= currentSample->loopEnd)
							{
								samplePingPong = false;
								samplePosition = (currentSample->loopEnd * 2) - samplePosition;
							}
							if (samplePosition >= currentSample->length)
							{
								samplePingPong = false;
								samplePosition = (float)(currentSample->length - 1);
							}
						}
						else
						{
							samplePosition -= sampleAdvance;

							if (samplePosition <= currentSample->loopStart)
							{
								samplePingPong = true;
								samplePosition = (currentSample->loopStart * 2) - samplePosition;
							}
							if (samplePosition < 0)
							{
								samplePingPong = true;
								samplePosition = 0;
							}
						}
						break;
					}
				}
			}
		}

		void channel::tick(const pattern* pat, const short row, const short tick, float* left, float* right, size_t samples)
		{
			const note* n = &pat->rows[row].notes[index];

			// handle new notes
			if (tick == 0)
			{
				handleInstrument(n);
				handleNote(n);
				handleVolumeColumn(n);
				handleEffectColumn(n);
			}

			if (currentInstrument)
			{
				handleEnvelope(&currentInstrument->volume, envelopeVolumeValue, envelopeVolumeTick);
				handleEnvelope(&currentInstrument->panning, envelopePanningValue, envelopePanningTick);
			}

			// autovibrato

			// arp

			// vibrato

			if (tick)
			{
				handleVolumeTick(n);
				handleEffectTick(n);
			}

			sampleFrequency = 8363 * powf(2, (6 * 12 * 16 * 4 - samplePeriod) / (12 * 16 * 4));
			sampleAdvance = sampleFrequency / 44100.0f;
			assert(sampleAdvance < 100);

			mix(left, right, samples);
		}

		player::player(document* doc)
			: doc(doc) 
			, currentBPM(doc->bpm)
			, currentTempo(doc->tempo)
		{ 
			for (short i = 0; i < doc->channels; ++i)
				channels.emplace_back(doc, i);
		}

		void player::chan(int channel)
		{
			currentChannel = channel;
		}
		
		void player::mix(short* buffer, size_t length)
		{
			size_t samplesTodo = length / 2;
			size_t offset = 0;

			while (samplesTodo)
			{
				if (samplesLeft == 0) tick();

				size_t n = std::min(samplesLeft, samplesTodo);

				for (size_t i = 0; i < n; ++i)
				{
					buffer[offset + i * 2 + 0] = (short)(gleft[i] * 0x1fff);
					buffer[offset + i * 2 + 1] = (short)(gright[i] * 0x1fff);
				}
				gleft = &gleft[n];
				gright = &gright[n];
				samplesTodo -= n;
				samplesLeft -= n;
				offset += n * 2;
			}
		}
		
		void player::tick()
		{
			samplesLeft = 110250 / currentBPM;

			left.assign(samplesLeft, 0);
			right.assign(samplesLeft, 0);
			gleft = &left[0];
			gright = &right[0];

			if (currentPattern >= doc->length)
				return;

			pattern* pat = &doc->patterns[doc->pattern_order[currentPattern]];

			currentTick++;
			if (currentTick >= currentTempo)
			{
				currentRow++;
				currentTick = 0;

				if (currentRow >= pat->rowCount)
				{
					currentPattern++;
					currentRow = 0;
					currentTick = 0;
					
					if (currentPattern >= doc->length)
						currentPattern = doc->restart_position;
				}
			}
			
			// make sure we sampling data from the right pattern
			pat = &doc->patterns[doc->pattern_order[currentPattern]];

			// mix in channels
			if (currentChannel == -1)
			{
				for (auto &c : channels)
				{
					c.tick(pat, currentRow, currentTick, left.data(), right.data(), samplesLeft);
				}
			}
			else
			{
				channels[currentChannel].tick(pat, currentRow, currentTick, left.data(), right.data(), samplesLeft);
			}
		}
	}
}