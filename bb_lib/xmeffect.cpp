#include <cassert>
#include "xmplay.h"

namespace bb
{
	namespace xm
	{
		void channel::handleEffectColumn(const note* note, player* play)
		{
			unsigned char x = (note->effectParameter & 0xf0) >> 4;
			unsigned char y = (note->effectParameter & 0x0f);

			switch (note->effect)
			{
			case 0x00: // arpeggio
				break;
			case 0x01: // porta up
				if (note->effectParameter)
					portamentoUp = note->effectParameter;
				break;
			case 0x02: // porta down
				if (note->effectParameter)
					portamentoDown = note->effectParameter;
				break;
			case 0x03:
				if (note->effectParameter)
					portamentoParam = note->effectParameter;
				break;
			case 0x04: // vibrato
				if (x) vibratoSpeed = x;
				if (y) vibratoDepth = y;
				break;
			case 0x05: // volume slide + tone portamento
				break;
			case 0x06: // volume slide + vibrato
				break;
			case 0x07: // tremolo
				if (x) tremoloSpeed = x;
				if (y) tremoloDepth = y;
				break;
			case 0x08: // set panning position
				panning = note->effectParameter / (float)0xff;
				break;
			case 0x09: // sample offset
				if (samplePosition == 0)
					samplePosition = (float)(note->effectParameter * 256);
				break;
			case 0x0a: // volume slide
				break;
			case 0x0b: // jump to order
				play->jump(note->effectParameter);
				break;
			case 0x0c: // set note volume
				volume = note->effectParameter / (float)0x40;
				break;
			case 0x0d: // pattern break
				play->patternBreak((note->effectParameter >> 4) * 10 + (note->effectParameter & 0x0F));
				break;
			case 0x0e: // extra control
				switch (x)
				{
				case 0x01: // fine portamento up
					samplePeriod -= 4 * y;
					break;
				case 0x02: // fine portamento down
					samplePeriod += 4 * y;
					break;
				case 0x05: // set note finetune
					samplePeriod -= (-0x8 + y) * (float)0x10;
					break;
				case 0x09: // retrigger note
					if (y) retriggerInterval = y;
					break;
				case 0x0c: // precision cut
					if (y == 0)
						cut();
					break;
				case 0x0d: // note delay
					if (note->pitch == 0 && note->instrument == 0) 
					{
						if (y)
						{
							pitch = pitchUnmodified;
							triggerNote(false, true, false);
						}
						else
						{
							triggerNote(true, true, true);
						}
					}
					break;
				default:
					assert(false && "effect not implemented");
					break;
				}
				break;
			case 0x0f: // set speed/bpm
				if (note->effectParameter < 0x20)
				{
					play->currentTempo = note->effectParameter;
				}
				else
				{
					play->currentBPM = note->effectParameter;
				}
				break;
			case 0x10: // set global volume
				play->currentVolume = fminf(1.0f, note->effectParameter / (float)0x40);
				break;
			case 0x11: // global volume slide
				if (x) globalVolumeSlide = +x / (float)0x40;
				if (y) globalVolumeSlide = -y / (float)0x40;
				if (x && y) globalVolumeSlide = 0;
				break;
			case 0x14: // key off
				release();
				break;
			case 0x15: // set volume envelope position
				envelopeVolumeTick = note->effectParameter;
				break;
			case 0x19: // panning slide
				break;
			case 0x1b: // retrigger note with volume slide
				if (x) retriggerVolume = x;
				if (y) retriggerInterval = y;
				break;
			case 0x1d: // tremor
				assert(false);
				break;
			case 0x21: // extra fine porta
				switch (x)
				{
				case 0x01: // porta up
					samplePeriod -= y;
					break;
				case 0x02: // porta down
					samplePeriod += y;
					break;
				default:
					assert(false && "effect not implemented");
					break;
				}
				break;
			default:
				assert(false && "effect not implemented");
				break;
			}
		}

		void channel::handleEffectTick(const note* note, player* play, const size_t tick)
		{
			unsigned char x = (note->effectParameter & 0xf0) >> 4;
			unsigned char y = (note->effectParameter & 0x0f);

			switch (note->effect)
			{
			case 0x00: // arpeggio
				arp0 = x;
				arp1 = y;
				break;
			case 0x01: // porta up
				samplePeriod -= 4 * portamentoUp;
				break;
			case 0x02: // porta down
				samplePeriod += 4 * portamentoDown;
				break;
			case 0x03: // tone porta
				handleTonePortamento();
				break;
			case 0x04: // vibrato
				handleVibrato();
				break;
			case 0x05: // volume slide + tone portamento
				volume = fminf(1.0f, volume + ((x) / (float)0x40));
				volume = fmaxf(0.0f, volume - ((y) / (float)0x40));
				handleTonePortamento();
				break;
			case 0x06:
				volume = fminf(1.0f, volume + ((x) / (float)0x40));
				volume = fmaxf(0.0f, volume - ((y) / (float)0x40));
				handleVibrato();
				break;
			case 0x07: // tremolo
				handleTremolo();
				break;
			case 0x08: // set panning
				break;
			case 0x09: // sample offset
				break;
			case 0x0a: // volume slide
				volume = fminf(1.0f, volume + ((x) / (float)0x40));
				volume = fmaxf(0.0f, volume - ((y) / (float)0x40));
				break;
			case 0x0b: // jump to order
				break;
			case 0x0c: // set note volume
				break;
			case 0x0d: // pattern break
				break;
			case 0x0e:
				switch (x)
				{
				case 0x01: // fine portamento up
					break;
				case 0x02: // fine portamento down
					break;
				case 0x05: // set note finetune
					break;
				case 0x09: // retrigger note
					if ((tick % retriggerInterval) == 0)
						triggerNote(false, false, false);
					break;
				case 0x0c: // precision cut
					if (y == tick)
						cut();
					break;
				case 0x0d: // note delay
					if (noteDelay == tick)
					{
						handleInstrument(note);
						handleNote(note);
						handleVolumeColumn(note);
						handleEffectColumn(note, play);
					}
					break;
				default:
					assert(false && "effect not implemented");
					break;
				}
				break;
			case 0x0f: // set speed/bpm
				break;
			case 0x10: // set global volume
				break;
			case 0x11: // global volume slide
				play->currentVolume = fmaxf(0.0f, fminf(1.0f, play->currentVolume + globalVolumeSlide));
				break;
			case 0x14: // key off
				break;
			case 0x15: // set volume envelope position
				break;
			case 0x19: // panning slide
				panning = fminf(1.0f, panning + ((x) / (float)0x40));
				panning = fmaxf(0.0f, panning - ((y) / (float)0x40));
				break;
			case 0x1b: // retrigger with volume slide
				switch (retriggerVolume)
				{
				case 0x01: volume = fmaxf(0.0f, volume - ( 1 / (float)0x40)); break;
				case 0x02: volume = fmaxf(0.0f, volume - ( 2 / (float)0x40)); break;
				case 0x03: volume = fmaxf(0.0f, volume - ( 4 / (float)0x40)); break;
				case 0x04: volume = fmaxf(0.0f, volume - ( 8 / (float)0x40)); break;
				case 0x05: volume = fmaxf(0.0f, volume - (16 / (float)0x40)); break;
				case 0x06: volume = fmaxf(0.0f, volume *   0.6666666666667f); break;
				case 0x07: volume = fmaxf(0.0f, volume *   0.5000000000000f); break;
				case 0x09: volume = fminf(1.0f, volume + ( 1 / (float)0x40)); break;
				case 0x0a: volume = fminf(1.0f, volume + ( 2 / (float)0x40)); break;
				case 0x0b: volume = fminf(1.0f, volume + ( 4 / (float)0x40)); break;
				case 0x0c: volume = fminf(1.0f, volume + ( 8 / (float)0x40)); break;
				case 0x0d: volume = fminf(1.0f, volume + (16 / (float)0x40)); break;
				case 0x0e: volume = fminf(1.0f, volume *   1.5000000000000f); break;
				case 0x0f: volume = fminf(1.0f, volume *   2.0000000000000f); break;
				default:                                                      break;
				}
				if ((tick % retriggerInterval) == 0)
				{
					float v = volume;
					triggerNote(false, false, false);
					if (currentInstrument)
					{
						handleEnvelope(&currentInstrument->volume, envelopeVolumeValue, envelopeVolumeTick);
						handleEnvelope(&currentInstrument->panning, envelopePanningValue, envelopePanningTick);
					}
					volume = v;
				}
				break;
			case 0x1d: // tremor
				assert(false);
				break;
			case 0x21: // extra fine portamento
				break;
			default:
				assert(false && "effect not implemented");
				break;
			}
		}
	}
}