
/*
 * fft.cpp - Tests for MilkDrop2 library's FFT.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>
#include <vis_milk2/fft.h>
#include <CppUnitTest.h>

using std::complex;
using std::vector;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MilkDrop2
{
std::default_random_engine randGen((std::random_device())());

TEST_CLASS(FftTest)
{
  private:
    double maxLogError = -INFINITY;

    void testFft(size_t n)
    {
        FFT mdfft{n, n, false, -1.0f};
        vector<float> fSpec;

        const vector<complex<float>> input = randomComplexes(n);
        const vector<complex<float>> expect = naiveDft(input, false);
        vector<float> input_real;
        for (auto i : input)
        {
            input_real.push_back(i.real());
        }
        vector<float> expect_real;
        for (auto i : expect)
        {
            expect_real.push_back(std::abs(i));
        }
        mdfft.TimeToFrequencyDomain(input_real, fSpec);
        double err = log10RmsErr(expect_real, fSpec);

        char buf[512];
        sprintf_s(buf, "fftsize=%4u logerr=%.3f\n", static_cast<unsigned int>(n), err); //cout << "fftsize=" << std::setw(4) << std::setfill(' ') << n << "  " << "logerr=" << std::setw(5) << std::setprecision(3) << std::setiosflags(std::ios::showpoint) << err << endl;
        Logger::WriteMessage(buf);
    }

    vector<complex<float>> naiveDft(const vector<complex<float>>& input, bool inverse)
    {
        int n = static_cast<int>(input.size());
        vector<complex<float>> output;
        double coef = (inverse ? 2.0 : -2.0) * M_PI / n;
        for (int k = 0; k < n; k++) // For each output element
        {
            complex<float> sum(0);
            for (int t = 0; t < n; t++) // For each input element
            {
                float theta = static_cast<float>(coef * (static_cast<long long>(t) * k % n));
                sum += input[t] * std::polar(1.0f, theta);
            }
            output.push_back(sum);
        }
        return output;
    }

    double log10RmsErr(const vector<float>& xvec, const vector<float>& yvec)
    {
        size_t n = xvec.size() / 8;
        double err = std::pow(10, -99 * 2);
        for (size_t i = 0; i < n; i++)
            err += std::norm(std::ceil(xvec.at(i) * 100000) / 100000 - std::ceil(yvec.at(i * 2) * 100000) / 100000);
        err /= n > 0 ? n : 1u;
        err = std::sqrt(err); // Now this is a root mean square (RMS) error
        err = std::log10(err);
        maxLogError = std::max(err, maxLogError);
        return err;
    }

    vector<complex<float>> randomComplexes(size_t n)
    {
        std::uniform_real_distribution<float> valueDist(-1.0, 1.0);
        vector<complex<float>> result;
        for (size_t i = 0; i < n; i++)
            result.push_back(complex<float>(valueDist(randGen), 0.0 /*valueDist(randGen)*/));
        return result;
    }

  public:
    FftTest()
    {
        Logger::WriteMessage("FftTest()\n");
    }

    ~FftTest()
    {
        Logger::WriteMessage("~FftTest()");
    }

    TEST_CLASS_INITIALIZE(FftTestInitialize)
    {
        Logger::WriteMessage("MilkDrop 2 FFT Test Class Initialize\n");
    }

    TEST_CLASS_CLEANUP(FftTestCleanup)
    {
        Logger::WriteMessage("MilkDrop 2 FFT Test Class Cleanup\n");
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(Fft2Test)
    TEST_OWNER(L"foo_vis_milk2")
    TEST_PRIORITY(1)
    END_TEST_METHOD_ATTRIBUTE()

    TEST_METHOD(Fft2Test)
    {
        Logger::WriteMessage("Smoke FFT Test\n");
        // Test power-of-2 size FFTs
        for (size_t i = 0; i <= 10; i++)
            testFft(static_cast<size_t>(1) << i);
        Assert::IsTrue(maxLogError < -4.5/*-10.0*/, L"Maximum logarithmic error is greater than -10.");
    }

    TEST_METHOD_INITIALIZE(MethodInit)
    {
        Logger::WriteMessage("MilkDrop 2 FFT Test Method Initialize\n");
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        Logger::WriteMessage("MilkDrop 2 FFT Test Method Cleanup\n");
    }
};
} // namespace MilkDrop2