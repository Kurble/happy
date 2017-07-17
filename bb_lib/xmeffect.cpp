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
			case 0x00: // do nothing
				break;
			case 0x01: // porta up
				break;
			case 0x02: // porta down
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
			case 0x09:
				cut();
				break;
			case 0x0a: // volume slide
				break;
			case 0x0b: // jump to order
				play->jump(note->effectParameter);
				break;
			case 0x0c: // set note volume
				volume = note->effectParameter / (float)0x40;
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
				play->currentVolume = note->effectParameter / (float)0x40;
				break;
			case 0x11: // global volume slide
				play->currentVolumeSlide = note->effectParameter / (float)0x40;
				break;
			default:
				assert(false && "effect not implemented");
				break;
			}
		}

		void channel::handleEffectTick(const note* note)
		{
			unsigned char x = (note->effectParameter & 0xf0) >> 4;
			unsigned char y = (note->effectParameter & 0x0f);

			switch (note->effect)
			{
			case 0x00: // do nothing
				break;
			case 0x01: // porta up
				samplePeriod -= 4 * note->effectParameter;
				break;
			case 0x02: // porta down
				samplePeriod += 4 * note->effectParameter;
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
			case 0x0a: // volume slide
				volume = fminf(1.0f, volume + ((x) / (float)0x40));
				volume = fmaxf(0.0f, volume - ((y) / (float)0x40));
				break;
			case 0x0b: // jump to order
				break;
			case 0x0c: // set note volume
				break;
			case 0x0f: // set speed/bpm
				break;
			case 0x10: // set global volume
				break;
			case 0x11: // global volume slide
				break;
			default:
				assert(false && "effect not implemented");
				break;
			}
		}
	}
}