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

#pragma once

#include <complex>
#include <vector>

class FFT
{
  public:
    FFT(size_t samplesIn, size_t samplesOut, bool equalize = true, float envelopePower = 1.0f);

    void TimeToFrequencyDomain(const std::vector<float>& waveformData, std::vector<float>& spectralData);

    size_t GetNumFrequencies() const { return m_numFrequencies; }

  private:
    void InitEqualizeTable(bool equalize);

    void InitEnvelopeTable(float power);

    void InitBitRevTable();

    void InitCosSinTable();

    size_t m_samplesIn; // Number of waveform samples to use for the FFT calculation.
    size_t m_numFrequencies; // Number of frequency samples calculated by the FFT.

    std::vector<size_t> m_bitRevTable; // Index table for frequency-specific waveform data lookups.
    std::vector<float> m_envelope; // Equalizer envelope table.
    std::vector<float> m_equalize; // Equalization values.
    std::vector<std::complex<float>> m_cosSinTable; // Table with complex polar coordinates for the different frequency domains used in the FFT.
};