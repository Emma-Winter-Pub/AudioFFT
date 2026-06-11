#pragma once

#include <QtGlobal>
#include <vector>

class IPlayerProvider {
public:
    virtual ~IPlayerProvider() = default;
    virtual qint64 readSamples(float* buffer, qint64 maxFloatCount) = 0;
    virtual void seek(double seconds) = 0;
    virtual double currentTime() const = 0;
    virtual bool atEnd() const = 0;
    virtual int getNativeSampleRate() const = 0;
    virtual void configureResampler(int targetRate) = 0;
    virtual double getLastDecodedTime() const = 0;
    virtual double getInternalBufferDuration() const = 0;
};