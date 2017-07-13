#include "xmplay.h"

#define HAS_TONE_PORTAMENTO(s) ((s)->effect == 3 || (s)->effect == 5 || ((s)->volume >> 4) == 0xF)

namespace bb
{
	namespace xm
	{
		channel::channel(document* doc, short index)
			: doc(doc)
			, index(index) { }

		void channel::handleNote(const note* note)
		{
			currentNote = note;
			currentInstrument = nullptr;
			currentSample = nullptr;

			// handle instrument
			if (currentNote->instrument)
			{
				if (currentNote->instrument <= doc->instrumentCount)
				{
					currentInstrument = &doc->instruments[currentNote->instrument - 1];

					if (currentNote->pitch == 0)
					{
						// ghost instrument
					}
				}
				else
				{
					cut();
				}
			}

			// handle note
			if (note->pitch > 0 && note->pitch < 97)
			{
				//
			}
			else if (note->pitch == 97)
			{
				release();
			}

			// handle volume
			switch (currentNote->volume)
			{
			default:
				break;
			}

			// handle effects
			switch (currentNote->effect)
			{
			default:
				break;
			}
		}

		void channel::tick(const pattern* pat, const short row, const short tick, short* buffer, size_t length)
		{
			// handle new notes
			if (tick == 0)
			{
				handleNote(&pat->rows[row].notes[index]);
			}

			// handle effects

			
			// mix into buffer
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
			
			buffer.clear();
			buffer.resize(110250 / currentBPM, 0);
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