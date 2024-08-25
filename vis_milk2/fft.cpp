/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "fft.h"
#include <cmath>

constexpr float PI = 3.141592653589793238462643383279502884197169399f;

// `samples_in`: number of waveform samples to feed into the FFT.
// `samples_out`: number of frequency samples desired; MUST BE A POWER OF 2.
// `bEqualize`: set to 1 if you want the magnitude of the basses and trebles
//              to be roughly equalized; 0 to leave them untouched.
// `envelope_power`: set to -1 to disable the envelope; otherwise, specify
//                   the envelope power desired. See `InitEnvelopeTable()`
//                   for more information.
FFT::FFT(size_t samplesIn, size_t samplesOut, bool equalize, float envelopePower) :
    m_samplesIn(samplesIn),
    m_numFrequencies(samplesOut * 2)
{
    InitBitRevTable();
    InitCosSinTable();
    InitEqualizeTable(equalize);
    InitEnvelopeTable(envelopePower);
}

void FFT::InitEqualizeTable(bool equalize)
{
    if (!equalize)
    {
        m_equalize = std::vector<float>(m_numFrequencies / 2, 1.0f);
        return;
    }

    const float scaling = -0.02f;
    const float inverseHalfNumFrequencies = 1.0f / static_cast<float>(m_numFrequencies / 2);

    m_equalize.resize(m_numFrequencies / 2);

    for (size_t i = 0; i < m_numFrequencies / 2; i++)
    {
        m_equalize[i] = scaling * std::log(static_cast<float>(m_numFrequencies / 2 - i) * inverseHalfNumFrequencies);
    }
}

// This precomputation is for multiplying the waveform sample
// by a bell-curve-shaped envelope, so we don't see the ugly
// frequency response (oscillations) of a square filter.
//
// A power of 1.0 will compute the FFT with exactly one convolution.
// A power of 2.0 is like doing it twice; the resulting frequency
// output will be smoothed out and the peaks will be "fatter".
// A power of 0.5 is closer to not using an envelope, and you start
// to get the frequency response of the square filter as 'power'
// approaches zero; the peaks get tighter and more precise, but
// you also see small oscillations around their bases.
void FFT::InitEnvelopeTable(float power)
{
    if (power < 0.0f)
    {
        // Keep all values as-is.
        m_envelope = std::vector<float>(m_samplesIn, 1.0f);
        return;
    }

    const float multiplier = 1.0f / static_cast<float>(m_samplesIn) * 2.0f * PI;

    m_envelope.resize(m_samplesIn);

    if (power == 1.0f)
    {
        for (size_t i = 0; i < m_samplesIn; i++)
        {
            m_envelope[i] = 0.5f + 0.5f * std::sin(static_cast<float>(i) * multiplier - PI * 0.5f);
        }
    }
    else
    {
        for (size_t i = 0; i < m_samplesIn; i++)
        {
            m_envelope[i] = std::pow(0.5f + 0.5f * std::sin(static_cast<float>(i) * multiplier - PI * 0.5f), power);
        }
    }
}

void FFT::InitBitRevTable()
{
    m_bitRevTable.resize(m_numFrequencies);

    for (size_t i = 0; i < m_numFrequencies; i++)
    {
        m_bitRevTable[i] = i;
    }

    size_t j = 0;
    for (size_t i = 0; i < m_numFrequencies; i++)
    {
        if (j > i)
        {
            const size_t temp = m_bitRevTable[i];
            m_bitRevTable[i] = m_bitRevTable[j];
            m_bitRevTable[j] = temp;
        }

        size_t m = m_numFrequencies >> 1;

        while (m >= 1 && j >= m)
        {
            j -= m;
            m >>= 1;
        }

        j += m;
    }
}

void FFT::InitCosSinTable()
{
    size_t tabsize = 0;
    size_t dftsize = 2;

    while (dftsize <= m_numFrequencies)
    {
        tabsize++;
        dftsize <<= 1;
    }

    m_cosSinTable.resize(tabsize);

    dftsize = 2;
    size_t i = 0;
    while (dftsize <= m_numFrequencies)
    {
        const float theta = static_cast<float>(-2.0f * PI / static_cast<float>(dftsize));
        m_cosSinTable[i] = std::polar(1.0f, theta); // Radius 1 is the unity circle.
        i++;
        dftsize <<= 1;
    }
}

// Converts time-domain samples from `in_wavedata[]`
// into frequency-domain samples in `out_spectraldata[]`.
// The array lengths are set at construction via parameters.
//
// The last sample of the output data will be the frequency
// that is 1/4th of the input sampling rate. For example,
// if the input wave data is sampled at 44,100 Hz, then the last
// sample of the spectral data output will represent the frequency
// 11,025 Hz. The first sample will be 0 Hz. The frequencies of
// the rest of the samples vary linearly in between.
//
// Note that since human hearing is limited to the range 200 - 20,000
// Hz, 200 Hz is a low bass hum and 20,000 Hz is an ear-piercing high shriek.
//
// Each time the frequency doubles, that sounds like going up an octave.
// That means that the difference between 200 and 300 Hz is FAR more
// than the difference between 5000 and 5100, for example!
// So, when trying to analyze bass, look at (probably)
// the 200 - 800 Hz range; whereas for treble, look at the 1,400 -
// 11,025 Hz range.
//
// To get 3 bands, try it this way:
//   a) 11,025 / 200 = 55.125
//   b) To get the number of octaves between 200 and 11,025 Hz, solve for n:
//          2^n = 55.125
//          n = log(55.125) / log(2)
//          n = 5.785
//   c) So each band should represent 5.785 / 3 = 1.928 octaves; their ranges are:
//          1) 200 - 200 * 2^1.928                        or  200  - 761   Hz
//          2) 200 * 2^1.928 - 200 * 2^(1.928 * 2)        or  761  - 2897  Hz
//          3) 200 * 2^(1.928 * 2) - 200 * 2^(1.928 * 3)  or  2897 - 11025 Hz
//
// A simple sine-wave-based envelope is convolved with the waveform
// data before doing the FFT to ameliorate the bad frequency response
// of a square (i.e., nonexistent) filter.
//
// If the input signal is not of a very high quality, dampening (blurring) it
// will reduce high-frequency noise that would otherwise show up in the output.
void FFT::TimeToFrequencyDomain(const std::vector<float>& waveformData, std::vector<float>& spectralData)
{
    if (m_bitRevTable.empty() || m_cosSinTable.empty() || waveformData.size() < m_samplesIn)
    {
        spectralData.clear();
        return;
    }

    // 1. Set up input to the FFT.
    std::vector<std::complex<float>> spectrumData(m_numFrequencies, std::complex<float>());
    for (size_t i = 0; i < m_numFrequencies; i++)
    {
        const size_t idx = m_bitRevTable[i];
        if (idx < m_samplesIn)
        {
            spectrumData[i].real(waveformData[idx] * m_envelope[idx]);
        }
    }

    // 2. Perform FFT.
    size_t dftSize = 2;
    size_t octave = 0;

    while (dftSize <= m_numFrequencies)
    {
        std::complex<float> w{1.0f, 0.0f};
        const std::complex<float> wp{m_cosSinTable[octave]};

        const size_t hdftsize = dftSize >> 1;

        for (size_t m = 0; m < hdftsize; m += 1)
        {
            for (size_t i = m; i < m_numFrequencies; i += dftSize)
            {
                const size_t j = i + hdftsize;
                const std::complex<float> tempNum = spectrumData[j] * w;
                spectrumData[j] = spectrumData[i] - tempNum;
                spectrumData[i] = spectrumData[i] + tempNum;
            }

            w *= wp;
        }

        dftSize <<= 1;
        octave++;
    }

    // 3. Take the magnitude and equalize it (on a log10 scale) for output.
    spectralData.resize(m_numFrequencies / 2);
    for (size_t i = 0; i < m_numFrequencies / 2; i++)
    {
        spectralData[i] = m_equalize[i] * std::abs(spectrumData[i]);
    }
}