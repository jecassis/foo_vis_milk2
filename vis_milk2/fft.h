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

/**
 * Performs a Fast Fourier Transform on audio sample data.
 * Also applies an equalizer pattern with an envelope curve to the resulting
 * data to smooth out certain artifacts.
 */
class FFT
{
  public:
    /**
     * Initializes the Fast Fourier Transform.
     *
     * \param[in] samplesIn Number of audio waveform samples which will be fed into the FFT.
     * \param[in] samplesOut Number of frequency samples generated; MUST BE A POWER OF 2.
     * \param[in] equalize Set to **true** to roughly equalize the magnitude of the basses and trebles,
     *                     **false** to leave them untouched. Defaults to **true**.
     * \param[in] envelopePower Specifies the envelope power. Set to any negative value to disable the envelope.
     *                          Defaults to **-1.0**. See `InitEnvelopeTable()` for more information.
     */
    FFT(size_t samplesIn, size_t samplesOut, bool equalize = true, float envelopePower = 1.0f);
    
    /**
     * Converts time-domain samples into frequency-domain samples.
     * The array lengths are the two parameters to the constructor.
     *
     * The last sample of the output data will be the frequency
     * that is 1/4th of the input sampling rate. For example,
     * if the input wave data is sampled at 44,100 Hz, then the last
     * sample of the spectral data output will represent the frequency
     * 11,025 Hz. The first sample will be 0 Hz. The frequencies of
     * the rest of the samples vary linearly in between.
     *
     * Note that since human hearing is limited to the range 20 – 20,000
     * Hz, 20 Hz is a low bass hum and 20,000 Hz is an ear-piercing high shriek.
     *
     * Each time the frequency doubles, that sounds like going up an octave.
     * That means that the difference between 200 and 300 Hz is FAR more
     * than the difference between 5000 and 5100 Hz, for example!
     * So, when trying to analyze bass, look at (probably)
     * the 200 – 800 Hz range; whereas for treble, look at the 1,400 –
     * 11,025 Hz range.
     *
     * To get 3 bands, try the following:
     *  1. 11,025 / 200 = 55.125
     *  2. To get the number of octaves between 200 and 11,025 Hz, solve for n:  
     *      2^n = 55.125  
     *      n = log(55.125) / log(2)  
     *      n = 5.785
     *  3. So each band should represent 5.785 / 3 = 1.928 octaves; their ranges are:
     *      1. 200 – 200 * 2^1.928                        or  200  – 761   Hz
     *      2. 200 * 2^1.928 – 200 * 2^(1.928 * 2)        or  761  – 2897  Hz
     *      3. 200 * 2^(1.928 * 2) – 200 * 2^(1.928 * 3)  or  2897 – 11025 Hz
     *
     * A simple sine-wave-based envelope is convolved with the waveform
     * data before doing the FFT to ameliorate the bad frequency response
     * of a square (i.e., nonexistent) filter.
     *
     * If the input signal is not of a very high quality, dampening (blurring) it
     * will reduce high-frequency noise that would otherwise show up in the output.
     *
     * \param[in]  waveformData The audio waveform data to convert. Must contain at least `samplesIn` number of elements.
     * \param[out] spectralData The resulting frequency data. Vector will be resized to `samplesOut` elements.
     *                          If the conversion fails the result vector will be empty (e.g., not initialized or too few input samples).
     */
    void TimeToFrequencyDomain(const std::vector<float>& waveformData, std::vector<float>& spectralData);

    /** 
     * Returns the number of frequency samples calculated.
     * This is twice the value of `samplesOut`.
     *
     * \return The number of frequency samples calculated.
     */
    size_t GetNumFrequencies() const { return m_numFrequencies; }

  private:
    /**
     * Calculates equalization factors for each frequency domain.
     *
     * \param[in] equalize If **false**, all factors will be set to one. Otherwise, will
     *                     calculate a frequency-based multiplier.
     */
    void InitEqualizeTable(bool equalize);

    /**
     * Initializes the equalizer envelope table.
     *
     * This pre-computation is for multiplying the waveform sample
     * by a bell-curve-shaped envelope, so there are no undesired
     * frequency response (oscillations) of a square filter.
     *
     *  - A `power` of **1.0** will compute the FFT with exactly one convolution.
     *  - A `power` of **2.0** is like doing it twice; the resulting frequency
     *    output will be smoothed out and the peaks will be "fatter".
     *  - A `power` of **0.5** is closer to not using an envelope. It look more
     *    like the frequency response of the square filter as `power`
     *    approaches zero; the peaks get tighter and more precise, but
     *    contain small oscillations around their bases.
     *
     * \param[in] power Number of convolutions in the envelope.
     */
    void InitEnvelopeTable(float power);

    /** Builds the sample lookup table for each octave. */
    void InitBitRevTable();

    /** Builds a table with the Nth roots of unity inputs for the transform. */
    void InitCosSinTable();

    size_t m_samplesIn; ///< Number of waveform samples to use for the FFT calculation.
    size_t m_numFrequencies; ///< Number of frequency samples calculated by the FFT.

    std::vector<size_t> m_bitRevTable; ///< Index table for frequency-specific waveform data lookups.
    std::vector<float> m_envelope; ///< Equalizer envelope table.
    std::vector<float> m_equalize; ///< Equalization values.
    std::vector<std::complex<float>> m_cosSinTable; ///< Table with complex polar coordinates for the different frequency domains used in the FFT.
};