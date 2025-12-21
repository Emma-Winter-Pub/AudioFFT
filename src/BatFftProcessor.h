#pragma once

#include <vector>


class BatFftProcessor{
    public:
        BatFftProcessor();
        ~BatFftProcessor();
        std::vector<std::vector<double>> compute(
            const std::vector<float>& pcmData,
            double timeInterval,
            int fftSize,
            int sampleRate);
    private:
};