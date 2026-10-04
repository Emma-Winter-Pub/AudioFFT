#pragma once

#include "BPBS_Types.h"
#include "BPBS_Asset.h"
#include "BPBS_DemandQueue.h"
#include "BPBS_Cache.h"
#include "BPBS_MetaCache.h"
#include "BPBS_CancellationToken.h"

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QElapsedTimer>
#include <QImage>
#include <atomic>
#include <memory>
#include <functional>

class BPBS_Decoder;
class BPBS_FFT;
class BPBS_Generator;

class BPBS_Worker : public QObject, public QRunnable {
    Q_OBJECT

public:
    using CalcParamsGetter = std::function<BPBS_CalcParams()>;
    using VisualParamsGetter = std::function<BPBS_VisualParams()>;
    explicit BPBS_Worker(int workerId,
                         int bucketIdPadding,
                         BPBS_DemandQueue& queue,
                         BPBS_Cache& cache,
                         BPBS_MetaCache& metaCache,
                         CalcParamsGetter calcParamsGetter,
                         VisualParamsGetter visualParamsGetter,
                         std::atomic<bool>* stopFlag,
                         std::atomic<bool>* pauseFlag,
                         QObject* parent = nullptr);
    ~BPBS_Worker() override = default;
    void run() override;

signals:
    void thumbnailReady(const QString& filePath);
    void metadataReady(const QString& filePath, const BPBS_AudioMetadata& info);
    void fileNotAudio(const QString& filePath);
    void p1FileReleased(const QString& filePath);
    void logMessage(const QString& msg);
    void bucketFinished(int bucketId, qint64 elapsedMs);

private:
    void checkPause();
    bool processSingleFile(const BPBS_DemandItem& item,
                           const BPBS_CalcParams& calcParams,
                           const BPBS_VisualParams& visualParams,
                           BPBS_FFT& fft,
                           BPBS_Generator& generator,
                           QString& errorMsg,
                           qint64& elapsedMsOut,
                           std::shared_ptr<BPBS_CancellationToken> token);
    QImage downsampleIndexed8(const QImage& src, int targetW, int targetH) const;
    int getRequiredFftSize(int height) const noexcept;
    static void splitPath(const QString& fullPath, QString& outDomain, QString& outFileName);
    const int m_workerId;
    const int m_bucketIdPadding;
    BPBS_DemandQueue& m_queue;
    BPBS_Cache& m_cache;
    BPBS_MetaCache& m_metaCache;
    CalcParamsGetter m_calcParamsGetter;
    VisualParamsGetter m_visualParamsGetter;
    std::atomic<bool>* m_stopFlag = nullptr;
    std::atomic<bool>* m_pauseFlag = nullptr;
};