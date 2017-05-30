#include "osha1stream.h"

osha1stream::osha1stream()
	: std::ostream((std::streambuf*)this)
{
	reset();
}

static uint32_t rol(const uint32_t value, const size_t bits)
{
	return (value << bits) | (value >> (32 - bits));
}
static uint32_t blk(const uint32_t block[16], const size_t i)
{
	return rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i], 1);
}
static void R0(const uint32_t block[16], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
	z += ((w&(x^y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
	w = rol(w, 30);
}
static void R1(uint32_t block[16], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
	block[i] = blk(block, i);
	z += ((w&(x^y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
	w = rol(w, 30);
}
static void R2(uint32_t block[16], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
	block[i] = blk(block, i);
	z += (w^x^y) + block[i] + 0x6ed9eba1 + rol(v, 5);
	w = rol(w, 30);
}
static void R3(uint32_t block[16], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
	block[i] = blk(block, i);
	z += (((w | x)&y) | (w&x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
	w = rol(w, 30);
}
static void R4(uint32_t block[16], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
	block[i] = blk(block, i);
	z += (w^x^y) + block[i] + 0xca62c1d6 + rol(v, 5);
	w = rol(w, 30);
}

static void transform(uint32_t digest[], uint32_t block[16], uint64_t &transforms)
{
	/* Copy digest[] to working vars */
	uint32_t a = digest[0];
	uint32_t b = digest[1];
	uint32_t c = digest[2];
	uint32_t d = digest[3];
	uint32_t e = digest[4];

	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(block, a, b, c, d, e, 0);
	R0(block, e, a, b, c, d, 1);
	R0(block, d, e, a, b, c, 2);
	R0(block, c, d, e, a, b, 3);
	R0(block, b, c, d, e, a, 4);
	R0(block, a, b, c, d, e, 5);
	R0(block, e, a, b, c, d, 6);
	R0(block, d, e, a, b, c, 7);
	R0(block, c, d, e, a, b, 8);
	R0(block, b, c, d, e, a, 9);
	R0(block, a, b, c, d, e, 10);
	R0(block, e, a, b, c, d, 11);
	R0(block, d, e, a, b, c, 12);
	R0(block, c, d, e, a, b, 13);
	R0(block, b, c, d, e, a, 14);
	R0(block, a, b, c, d, e, 15);
	R1(block, e, a, b, c, d, 0);
	R1(block, d, e, a, b, c, 1);
	R1(block, c, d, e, a, b, 2);
	R1(block, b, c, d, e, a, 3);
	R2(block, a, b, c, d, e, 4);
	R2(block, e, a, b, c, d, 5);
	R2(block, d, e, a, b, c, 6);
	R2(block, c, d, e, a, b, 7);
	R2(block, b, c, d, e, a, 8);
	R2(block, a, b, c, d, e, 9);
	R2(block, e, a, b, c, d, 10);
	R2(block, d, e, a, b, c, 11);
	R2(block, c, d, e, a, b, 12);
	R2(block, b, c, d, e, a, 13);
	R2(block, a, b, c, d, e, 14);
	R2(block, e, a, b, c, d, 15);
	R2(block, d, e, a, b, c, 0);
	R2(block, c, d, e, a, b, 1);
	R2(block, b, c, d, e, a, 2);
	R2(block, a, b, c, d, e, 3);
	R2(block, e, a, b, c, d, 4);
	R2(block, d, e, a, b, c, 5);
	R2(block, c, d, e, a, b, 6);
	R2(block, b, c, d, e, a, 7);
	R3(block, a, b, c, d, e, 8);
	R3(block, e, a, b, c, d, 9);
	R3(block, d, e, a, b, c, 10);
	R3(block, c, d, e, a, b, 11);
	R3(block, b, c, d, e, a, 12);
	R3(block, a, b, c, d, e, 13);
	R3(block, e, a, b, c, d, 14);
	R3(block, d, e, a, b, c, 15);
	R3(block, c, d, e, a, b, 0);
	R3(block, b, c, d, e, a, 1);
	R3(block, a, b, c, d, e, 2);
	R3(block, e, a, b, c, d, 3);
	R3(block, d, e, a, b, c, 4);
	R3(block, c, d, e, a, b, 5);
	R3(block, b, c, d, e, a, 6);
	R3(block, a, b, c, d, e, 7);
	R3(block, e, a, b, c, d, 8);
	R3(block, d, e, a, b, c, 9);
	R3(block, c, d, e, a, b, 10);
	R3(block, b, c, d, e, a, 11);
	R4(block, a, b, c, d, e, 12);
	R4(block, e, a, b, c, d, 13);
	R4(block, d, e, a, b, c, 14);
	R4(block, c, d, e, a, b, 15);
	R4(block, b, c, d, e, a, 0);
	R4(block, a, b, c, d, e, 1);
	R4(block, e, a, b, c, d, 2);
	R4(block, d, e, a, b, c, 3);
	R4(block, c, d, e, a, b, 4);
	R4(block, b, c, d, e, a, 5);
	R4(block, a, b, c, d, e, 6);
	R4(block, e, a, b, c, d, 7);
	R4(block, d, e, a, b, c, 8);
	R4(block, c, d, e, a, b, 9);
	R4(block, b, c, d, e, a, 10);
	R4(block, a, b, c, d, e, 11);
	R4(block, e, a, b, c, d, 12);
	R4(block, d, e, a, b, c, 13);
	R4(block, c, d, e, a, b, 14);
	R4(block, b, c, d, e, a, 15);

	// Add the working vars back into digest[]
	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;

	// Count the number of transformations
	transforms++;
}

void osha1stream::reset()
{
	m_digest.digest[0] = 0x67452301;
	m_digest.digest[1] = 0xefcdab89;
	m_digest.digest[2] = 0x98badcfe;
	m_digest.digest[3] = 0x10325476;
	m_digest.digest[4] = 0xc3d2e1f0;
	m_transforms = 0;
	setp(m_buffer, m_buffer, m_buffer + 64);
}

sha1 osha1stream::hash()
{
	uint64_t total_bits = (m_transforms*64 + (pptr() - pbase())) * 8;

	put((char)0x80);
	
	size_t orig_size = (pptr() - pbase());
	for (char* buf = pptr(); buf != pbase() + 64; ++buf)
	{
		*buf = 0x00;
	}
	
	if (orig_size > 56)
	{
		process();
		for (size_t i = 0; i < 14; i++)
		{
			m_block[i] = 0;
		}
	}

	// Append total_bits, split this uint64_t into two uint32_t
	m_block[15] = (total_bits >> 00);
	m_block[14] = (total_bits >> 32);
	process();

	sha1 result = m_digest;
	reset();
	return result;
}

void osha1stream::process()
{
	for (int i = 0; i < 16; ++i)
	{
		m_block[i] = (m_buffer[4 * i + 3] & 0xff)
			       | (m_buffer[4 * i + 2] & 0xff) << 8
			       | (m_buffer[4 * i + 1] & 0xff) << 16
			       | (m_buffer[4 * i + 0] & 0xff) << 24;
	}
	transform(m_digest.digest, m_block, m_transforms);
}

int osha1stream::overflow(int c)
{
	process();
		
	m_buffer[0] = std::streambuf::traits_type::to_char_type(c);

	setp(m_buffer, m_buffer + 1, m_buffer + 64);

	return std::streambuf::traits_type::not_eof(c);
}