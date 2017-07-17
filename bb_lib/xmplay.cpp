#include <cassert>
#include "xmplay.h"

#define HAS_TONE_PORTAMENTO(s) ()

namespace bb
{
	namespace xm
	{
		inline bool hasTonePortamento(const note* note) 
		{ 
			return note->effect == 3 || note->effect == 5 || (note->volume & 0xf0) == 0xF0; 
		}

		inline float waveform(unsigned char waveform, unsigned char step)
		{
			step %= 0x40;
			switch (waveform & 0x03)
			{
			case 0: // sine
				return -sinf((step/(float)0x40)*6.283185f);
			case 1: // ramp down
				return (float)(0x20 - step) / (float)0x20;
			case 2: // square
				return (step >= 0x20) ? 1.f : -1.f;

			}
			return 0;
		}

		channel::channel(document* doc, short index)
			: doc(doc)
			, index(index) { }

		void channel::handleInstrument(const note* note)
		{
			if (note->instrument)
			{
				if (currentInstrument && currentSample && hasTonePortamento(note))
				{
					triggerNote(true, false, true);
				}
				else if (note->instrument <= doc->instrumentCount)
				{
					currentInstrument = &doc->instruments[note->instrument - 1];
					if (note->pitch == 0 && currentSample)
					{
						triggerNote(true, false, false);
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

		void channel::handleVibrato()
		{
			size_t step = vibratoTick * vibratoSpeed;
			vibratoTick++;

			vibratoOffset = 2.0f * waveform(vibratoWave, (unsigned char)(step % 0x40)) * (vibratoDepth/(float)0x0f);
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
			fadeoutVolume = 1;
			sustained = true;

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
			sustained = false;

			if (currentInstrument == nullptr || (currentInstrument->volume.type & 0x01) == 0)
			{
				cut();
			}
		}

		void channel::handleEnvelope(const envelope* env, float& val, size_t& tick)
		{
			if (env->type & 0x01) // 1 flag: env is enable
			{
				if (env->type & 0x04) // 4 flag: looping
				{
					if (env->loopStart == env->loopEnd)
						tick = env->loopStart;

					if (tick >= env->points[env->loopEnd].frame)
						tick -= env->points[env->loopEnd].frame - env->points[env->loopStart].frame;
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

				if (!sustained || (env->type & 0x02) == 0 || tick != env->sustainPoint)
				{
					tick++;
				}
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
				float finalVolume = fadeoutVolume * envelopeVolumeValue * volume;
				float volLeft = (1 - panning)*finalVolume;
				float volRight = (panning)*finalVolume;

				for (size_t i = 0; i < samples; ++i)
				{
					if (samplePosition < 0)
					{
						continue;
					}

					size_t a = (size_t)samplePosition;
					size_t b = a + 1;
					float t = samplePosition - a;
					float u = currentSample->samples[a];
					float v = 0;

					switch (currentSample->type & 0x3)
					{
					case 0: // no loop
						v = (b >= currentSample->length) ? 0.0f : currentSample->samples[b];

						samplePosition += sampleAdvance;
						if (samplePosition >= currentSample->length)
							samplePosition = -1;
						break;

					case 1: // forward loop
						v = (b >= currentSample->loopEnd) ? currentSample->samples[currentSample->loopStart] : currentSample->samples[b];

						samplePosition += sampleAdvance;
						while (samplePosition >= currentSample->loopEnd)
							samplePosition -= (currentSample->loopEnd - currentSample->loopStart);
						break;

					case 2: // pingpong loop
						if (samplePingPong)
						{
							v = (b >= currentSample->loopEnd) ? currentSample->samples[a] : currentSample->samples[b];
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
							v = u;
							u = (b == 1 || b - 2 <= currentSample->loopStart) ? currentSample->samples[a] : currentSample->samples[b - 2];

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
				
					left[i]  += volLeft  * (u+(v-u)*t);
					right[i] += volRight * (u+(v-u)*t);
				}
			}
		}

		void channel::tick(const pattern* pat, player* play, const short row, const short tick, float* left, float* right, size_t samples)
		{
			const note* n = &pat->rows[row].notes[index];

			// handle new notes
			if (tick == 0)
			{
				vibratoOffset = 0;

				handleInstrument(n);
				handleNote(n);
				handleVolumeColumn(n);
				handleEffectColumn(n, play);
			}

			if (currentInstrument)
			{
				handleEnvelope(&currentInstrument->volume, envelopeVolumeValue, envelopeVolumeTick);
				handleEnvelope(&currentInstrument->panning, envelopePanningValue, envelopePanningTick);

				if (!sustained)
				{
					fadeoutVolume = fmaxf(0, fadeoutVolume - (currentInstrument->volumeFadeout / (float)0x8000));
				}
			}

			// autovibrato

			// arp

			// vibrato

			if (tick > 0)
			{
				handleVolumeTick(n);
				handleEffectTick(n, play);
			}

			// LINEAR frequencies
			float period = samplePeriod - 64.0f * (vibratoOffset + autovibratoOffset);
			if (tick % 3 == 1) period -= 64.0f * arp0;
			if (tick % 3 == 2) period -= 64.0f * arp1;
			sampleFrequency = 8363 * powf(2, (6 * 12 * 16 * 4 - period) / (12 * 16 * 4));
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
					buffer[offset + i * 2 + 0] = (short)(gleft[i] * 0x1fff * currentVolume);
					buffer[offset + i * 2 + 1] = (short)(gright[i] * 0x1fff * currentVolume);
				}
				gleft = &gleft[n];
				gright = &gright[n];
				samplesTodo -= n;
				samplesLeft -= n;
				offset += n * 2;
			}
		}
		
		void player::jump(unsigned char pattern)
		{
			jumpToPattern = pattern;
		}

		void player::patternBreak(unsigned char row)
		{
			jumpToRow = row;
		}

		void player::tick()
		{
			samplesLeft = 110250 / currentBPM;

			if (currentPattern >= doc->length)
				return;

			pattern* pat = &doc->patterns[doc->pattern_order[currentPattern]];

			if (currentTick >= currentTempo)
			{
				currentRow++;
				currentTick = 0;

				if (currentRow >= pat->rowCount || jumpToRow < 0xffff)
				{
					currentPattern++;
					currentRow = jumpToRow < 0xffff ? jumpToRow : 0;
					currentTick = 0;
					
					if (currentPattern >= doc->length)
						currentPattern = doc->restart_position;
				}

				jumpToRow = 0xffff;
			}
			
			do
			{
				left.assign(samplesLeft, 0);
				right.assign(samplesLeft, 0);
				gleft = &left[0];
				gright = &right[0];

				// make sure we sampling data from the right pattern
				pat = &doc->patterns[doc->pattern_order[currentPattern]];

				jumpToPattern = 0xffff;

				// mix in channels
				if (currentChannel == -1)
				{
					for (auto &c : channels)
					{
						c.tick(pat, this, currentRow, currentTick, left.data(), right.data(), samplesLeft);
					}
				}
				else
				{
					channels[currentChannel].tick(pat, this, currentRow, currentTick, left.data(), right.data(), samplesLeft);
				}

				if (jumpToPattern < 0xffff)
				{
					currentPattern = jumpToPattern;
					currentRow = 0;
					currentTick = 0;
				}
			} while (jumpToPattern < 0xffff);

			currentTick++;
		}
	}
}