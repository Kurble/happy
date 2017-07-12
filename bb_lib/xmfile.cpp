#include "xmfile.h"

namespace bb
{
	namespace xm
	{
		channel::channel(document* doc, short index)
			: doc(doc)
			, index(index) { }

		void channel::tick(const pattern* pat, const short row, const short tick, const short* buffer, size_t length)
		{
			//
		}

		player::player(document* doc)
			: doc(doc) 
			, currentBPM(doc->bpm)
			, currentTempo(doc->tempo)
		{ 
			for (short i = 0; i < doc->channels; ++i)
				channels.emplace_back(doc, i);
		}
		
		bool player::tick(short** _buffer, size_t* _length)
		{
			if (currentPattern >= doc->length)
				return false;
			
			buffer.resize(110250 / currentBPM);
			*_buffer = buffer.data();
			*_length = buffer.size();

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
						return false;
				}
			}
			
			// make sure we sampling data from the right pattern
			pat = &doc->patterns[doc->pattern_order[currentPattern]];

			// mix in channels
			for (auto &c : channels)
			{
				c.tick(pat, currentRow, currentTick, *_buffer, *_length);
			}

			return true;
		}
	}
}