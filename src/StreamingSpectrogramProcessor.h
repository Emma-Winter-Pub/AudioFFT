#pragma once

#include "StreamingTypes.h"
#include "FFTWindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"
#include "CueParser.h"

#include <QThread>
#include <QImage>
#include <atomic>
#include <mutex>
#include <optional> 

class StreamingSpectrogramProcessor : public QThread{
    Q_OBJECT

public:
    explicit StreamingSpectrogramProcessor(QObject* parent = nullptr);
    ~StreamingSpectrogramProcessor();
    void startProcessing(const QString& filePath,
                         double timeInterval, int imageHeight, int fftSize,
                         CurveType curveType, double minDb, double maxDb,
                         const QString& paletteId, bool paletteInverted, bool paletteNegative,
                         FFTWindowType windowType,
                         int trackIdx, int channelIdx,
                         double startSec = 0.0, double endSec = 0.0,
                         std::optional<CueSheet> cueSheet = std::nullopt);
    void stopProcessing();

signals:
    void logMessage(const QString& msg);
    void processingStarted(const StreamingTypes::StreamingAudioMetadata& metadata);
    void processingFinished(double realDuration);
    void processingFailed(const QString& errMsg);
    void chunkReady(const QImage& chunk, double startTimeSec, double durationSec);

protected:
    void run() override;

private:
    QString m_filePath;
    double m_timeInterval;
    int m_imageHeight;
    int m_fftSize;
    CurveType m_curveType;
    double m_minDb;
    double m_maxDb;
    QString m_paletteId;
    bool m_paletteInverted = false;
    bool m_paletteNegative = false;
    FFTWindowType m_windowType;
    int m_trackIdx;
    int m_channelIdx;
    double m_startSec = 0.0;
    double m_endSec = 0.0;
    std::optional<CueSheet> m_cueSheet; 
    std::atomic<bool> m_stopRequested{false};
    std::mutex m_mutex;
    template <typename FftEngine, typename PcmType>
    double processLoop(FftEngine& fftEngine, class StreamingAudioDecoder& decoder);
};