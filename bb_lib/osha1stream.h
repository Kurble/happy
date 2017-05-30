#pragma once

#include <streambuf>
#include <ostream>

struct sha1
{
	uint32_t digest[5];
};

class osha1stream : private std::streambuf, public std::ostream
{
public:
	osha1stream();

	void reset();

	sha1 hash();

private:
	void process();

	virtual int overflow(int c = EOF) override;

	union
	{
		char     m_buffer[64];
		uint32_t m_block[16];
	};

	sha1 m_digest;

	uint64_t m_transforms;
};