/* eos - A reimplementation of BioWare's Aurora engine
 *
 * eos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

// Based upon the (I)FFT code in FFmpeg
// Copyright (c) 2008 Loren Merritt
// Copyright (c) 2002 Fabrice Bellard
// Partly based on libdjbfft by D. J. Bernstein

/** @file common/fft.cpp
 *  (Inverse) Fast Fourier Transform.
 */

#include <cassert>
#include <cstring>

#include "common/maths.h"
#include "common/cosinetables.h"
#include "common/util.h"
#include "common/fft.h"

namespace Common {

FFT::FFT(int bits, bool inverse) : _bits(bits), _inverse(inverse) {
	assert((_bits >= 2) && (_bits <= 16));

	int n = 1 << bits;

	_tmpBuf = new Complex[n];
	_expTab = new Complex[n / 2];
	_revTab = new uint16[n];

	_splitRadix = 1;

	for (int i = 0; i < n; i++)
		_revTab[-splitRadixPermutation(i, n, _inverse) & (n - 1)] = i;
}

FFT::~FFT() {
	delete[] _revTab;
	delete[] _expTab;
	delete[] _tmpBuf;
}

const uint16 *FFT::getRevTab() const {
	return _revTab;
}

void FFT::permute(Complex *z) {
	int np = 1 << _bits;

	if (_tmpBuf) {
		for(int j = 0; j < np; j++)
			_tmpBuf[_revTab[j]] = z[j];

		std::memcpy(z, _tmpBuf, np * sizeof(Complex));

		return;
	}

	// Reverse
	for(int j = 0; j < np; j++) {
		int k = _revTab[j];

		if (k < j)
			SWAP(z[k], z[j]);
	}
}

int FFT::splitRadixPermutation(int i, int n, bool inverse) {
	if (n <= 2)
		return i & 1;

	int m = n >> 1;

	if(!(i & m))
		return splitRadixPermutation(i, m, inverse) * 2;

	m >>= 1;

	if(inverse == !(i & m))
		return splitRadixPermutation(i, m, inverse) * 4 + 1;

	return splitRadixPermutation(i, m, inverse) * 4 - 1;
}

#define sqrthalf (float)M_SQRT1_2

#define BF(x,y,a,b) {\
	x = a - b;\
	y = a + b;\
}

#define BUTTERFLIES(a0,a1,a2,a3) {\
	BF(t3, t5, t5, t1);\
	BF(a2.re, a0.re, a0.re, t5);\
	BF(a3.im, a1.im, a1.im, t3);\
	BF(t4, t6, t2, t6);\
	BF(a3.re, a1.re, a1.re, t4);\
	BF(a2.im, a0.im, a0.im, t6);\
}

// force loading all the inputs before storing any.
// this is slightly slower for small data, but avoids store->load aliasing
// for addresses separated by large powers of 2.
#define BUTTERFLIES_BIG(a0,a1,a2,a3) {\
	float r0=a0.re, i0=a0.im, r1=a1.re, i1=a1.im;\
	BF(t3, t5, t5, t1);\
	BF(a2.re, a0.re, r0, t5);\
	BF(a3.im, a1.im, i1, t3);\
	BF(t4, t6, t2, t6);\
	BF(a3.re, a1.re, r1, t4);\
	BF(a2.im, a0.im, i0, t6);\
}

#define TRANSFORM(a0,a1,a2,a3,wre,wim) {\
	t1 = a2.re * wre + a2.im * wim;\
	t2 = a2.im * wre - a2.re * wim;\
	t5 = a3.re * wre - a3.im * wim;\
	t6 = a3.im * wre + a3.re * wim;\
	BUTTERFLIES(a0,a1,a2,a3)\
}

#define TRANSFORM_ZERO(a0,a1,a2,a3) {\
	t1 = a2.re;\
	t2 = a2.im;\
	t5 = a3.re;\
	t6 = a3.im;\
	BUTTERFLIES(a0,a1,a2,a3)\
}

/* z[0...8n-1], w[1...2n-1] */
#define PASS(name)\
static void name(Complex *z, const float *wre, unsigned int n)\
{\
	float t1, t2, t3, t4, t5, t6;\
	int o1 = 2*n;\
	int o2 = 4*n;\
	int o3 = 6*n;\
	const float *wim = wre+o1;\
	n--;\
\
	TRANSFORM_ZERO(z[0],z[o1],z[o2],z[o3]);\
	TRANSFORM(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
	do {\
		z += 2;\
		wre += 2;\
		wim -= 2;\
		TRANSFORM(z[0],z[o1],z[o2],z[o3],wre[0],wim[0]);\
		TRANSFORM(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
	} while(--n);\
}

PASS(pass)
#undef BUTTERFLIES
#define BUTTERFLIES BUTTERFLIES_BIG
PASS(pass_big)

#define DECL_FFT(t,n,n2,n4)\
static void fft##n(Complex *z)\
{\
	fft##n2(z);\
	fft##n4(z+n4*2);\
	fft##n4(z+n4*3);\
	pass(z,getCosineTable(t),n4/2);\
}

static void fft4(Complex *z)
{
	float t1, t2, t3, t4, t5, t6, t7, t8;

	BF(t3, t1, z[0].re, z[1].re);
	BF(t8, t6, z[3].re, z[2].re);
	BF(z[2].re, z[0].re, t1, t6);
	BF(t4, t2, z[0].im, z[1].im);
	BF(t7, t5, z[2].im, z[3].im);
	BF(z[3].im, z[1].im, t4, t8);
	BF(z[3].re, z[1].re, t3, t7);
	BF(z[2].im, z[0].im, t2, t5);
}

static void fft8(Complex *z)
{
	float t1, t2, t3, t4, t5, t6, t7, t8;

	fft4(z);

	BF(t1, z[5].re, z[4].re, -z[5].re);
	BF(t2, z[5].im, z[4].im, -z[5].im);
	BF(t3, z[7].re, z[6].re, -z[7].re);
	BF(t4, z[7].im, z[6].im, -z[7].im);
	BF(t8, t1, t3, t1);
	BF(t7, t2, t2, t4);
	BF(z[4].re, z[0].re, z[0].re, t1);
	BF(z[4].im, z[0].im, z[0].im, t2);
	BF(z[6].re, z[2].re, z[2].re, t7);
	BF(z[6].im, z[2].im, z[2].im, t8);

	TRANSFORM(z[1],z[3],z[5],z[7],sqrthalf,sqrthalf);
}

static void fft16(Complex *z)
{
	float t1, t2, t3, t4, t5, t6;

	fft8(z);
	fft4(z+8);
	fft4(z+12);

	const float * const cosTable = getCosineTable(4);

	TRANSFORM_ZERO(z[0],z[4],z[8],z[12]);
	TRANSFORM(z[2],z[6],z[10],z[14],sqrthalf,sqrthalf);
	TRANSFORM(z[1],z[5],z[9],z[13],cosTable[1],cosTable[3]);
	TRANSFORM(z[3],z[7],z[11],z[15],cosTable[3],cosTable[1]);
}

DECL_FFT(5, 32,16,8)
DECL_FFT(6, 64,32,16)
DECL_FFT(7, 128,64,32)
DECL_FFT(8, 256,128,64)
DECL_FFT(9, 512,256,128)
#define pass pass_big
DECL_FFT(10, 1024,512,256)
DECL_FFT(11, 2048,1024,512)
DECL_FFT(12, 4096,2048,1024)
DECL_FFT(13, 8192,4096,2048)
DECL_FFT(14, 16384,8192,4096)
DECL_FFT(15, 32768,16384,8192)
DECL_FFT(16, 65536,32768,16384)

static void (* const fft_dispatch[])(Complex*) = {
	fft4, fft8, fft16, fft32, fft64, fft128, fft256, fft512, fft1024,
	fft2048, fft4096, fft8192, fft16384, fft32768, fft65536,
};

void FFT::calc(Complex *z) {
	fft_dispatch[_bits - 2](z);
}

} // End of namespace Common
