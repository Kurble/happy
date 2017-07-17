#include <cassert>
#include "xmplay.h"

namespace bb
{
	namespace xm
	{
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
			case 0x60: // volume slide down
				break;
			case 0x70: // volume slide up
				break;
			case 0x80: // fine volume slide down
				volume -= (note->volume & 0x0f) / (float)0x40;
				break;
			case 0x90: // fine volume slide up
				volume += (note->volume & 0x0f) / (float)0x40;
				break;
			case 0xa0: // vibrato speed
				vibratoSpeed = note->volume & 0x0f;
				break;
			case 0xb0: // vibrato depth
				vibratoDepth = note->volume & 0x0f;
				break;
			case 0xc0: // set panning
				panning = (note->volume & 0x0f | ((note->volume & 0x0f) << 4)) / (float)0xff;
				break;
			case 0xd0: // panning slide left
				break;
			case 0xe0: // panning slide right
				break;
			case 0xf0: // tone portamento
				if (note->volume & 0x0f)
				{
					portamentoParam = (note->volume & 0x0f) | ((note->volume & 0x0f) << 4);
				}
				break;
			default: // don't know
				assert(false && "effect not implemented");
				break;
			}
		}

		void channel::handleVolumeTick(const note* note)
		{
			switch (note->volume & 0xf0)
			{
			case 0x00: // do nothing
				break;
			case 0x10:
			case 0x20:
			case 0x30:
			case 0x40:
			case 0x50: // set volume
				break;
			case 0x60: // volume slide down
				volume = fmaxf(0.0f, volume - ((note->volume & 0x0f) / (float)0x40));
				break;
			case 0x70: // volume slide up
				volume = fminf(1.0f, volume + ((note->volume & 0x0f) / (float)0x40));
				break;
			case 0x80: // fine volume slide down
				break;
			case 0x90: // fine volume slide up
				break;
			case 0xa0: // vibrato speed
			case 0xb0: // vibrato depth
				handleVibrato();
				break;
			case 0xc0: // set pan
				break;
			case 0xd0: // pan slide left
				volume = fmaxf(0.0f, panning - ((note->volume & 0x0f) / (float)0x80));
				break;
			case 0xe0: // pan slide right
				volume = fminf(1.0f, panning + ((note->volume & 0x0f) / (float)0x80));
				break;
			case 0xf0: // tone portamento
				handleTonePortamento();
				break;
			default:
				assert(false && "effect not implemented"); assert(false);
				break;
			}
		}
	}
}