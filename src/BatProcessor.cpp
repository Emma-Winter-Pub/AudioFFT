#include "BatProcessor.h"
#include "BatWorker.h"
#include "BatUtils.h"
#include "BatFFmpegRAII.h"

#include <mutex>
#include <cmath>
#include <numeric>
#include <QMap>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>


const QMap<AVCodecID, int> CODEC_ID_WEIGHTS = {

    {AV_CODEC_ID_APE,            10},
    {AV_CODEC_ID_AAC,             5},
    {AV_CODEC_ID_DSD_LSBF,        4},
    {AV_CODEC_ID_DSD_MSBF,        4},
    {AV_CODEC_ID_DSD_LSBF_PLANAR, 4},
    {AV_CODEC_ID_DSD_MSBF_PLANAR, 4},
    {AV_CODEC_ID_FLAC,            3},
    {AV_CODEC_ID_ALAC,            3},
    {AV_CODEC_ID_WAVPACK,         3},
    {AV_CODEC_ID_WMALOSSLESS,     3},
    {AV_CODEC_ID_VORBIS,          2},
    {AV_CODEC_ID_OPUS,            2},
    {AV_CODEC_ID_WMAV2,           2},
    {AV_CODEC_ID_MP3,             5},
    {AV_CODEC_ID_MP2,             5},
    {AV_CODEC_ID_PCM_S16LE,       1},
    {AV_CODEC_ID_PCM_S24LE,       1},
    {AV_CODEC_ID_PCM_S32LE,       1},
    {AV_CODEC_ID_PCM_F32LE,       1},
    {AV_CODEC_ID_PCM_F64LE,       1}

};


const int DEFAULT_WEIGHT = 1;


BatProcessor::BatProcessor(QObject *parent) 
    : QObject(parent)
{
    m_stopFlag = false;
    m_pauseFlag = false;
    m_totalFileCount = 0;
    m_processedFileCount = 0;
    m_runningWorkers = 0;
    m_bucketIdPadding = 1;
}


BatProcessor::~BatProcessor(){

    if (m_runningWorkers.load() > 0) { stop(); }
    cleanup();
}


void BatProcessor::processBatch(const BatSettings& settings){

    if (m_runningWorkers.load() > 0) {
        emit logMessage(BatUtils::getCurrentTimestamp() + tr(" [Error] A task is already running. Please wait for it to complete before starting a new one."));
        return;}
    
    cleanup();

    m_currentSettings    = settings;
    m_stopFlag           = false;
    m_pauseFlag          = false;
    m_processedFileCount = 0;
    m_failedFiles.clear();
    m_allFileMetrics.clear();
    m_bucketCompletionTimes.clear();

    m_totalTaskTimer.start();
    emit batchStarted();
    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Task started. Scanning files..."));

    QList<FileInfo> allFiles = scanAndProbeFiles(settings.inputPath, settings.includeSubfolders);

    if (m_stopFlag.load()) {
        emit batchStopped();
        return;}

    if (allFiles.isEmpty()) {
        emit logMessage(BatUtils::getCurrentTimestamp() + tr(" No files containing audio streams were found."));
        emit batchFinished(tr("Task completed, but no files were processed."));
        return;}

    m_totalFileCount = allFiles.size();
    int totalCount = m_totalFileCount.load();
    emit logMessage(BatUtils::getCurrentTimestamp() + QString(tr(" Scan complete. Found %1 valid audio file(s).")).arg(totalCount));

    startWorkers(allFiles, settings);
}


QList<FileInfo> BatProcessor::scanAndProbeFiles(const QString& rootPath, bool includeSubfolders){

    QList<FileInfo> validFiles;
    QDirIterator::IteratorFlags flags = includeSubfolders ? 
                                        QDirIterator::Subdirectories : 
                                        QDirIterator::NoIteratorFlags;
    QDirIterator it(rootPath, QDir::Files, flags);
    while (it.hasNext()) {
        if (m_stopFlag.load()) break;
        QString path = it.next();
        auto fileInfoOpt = probeFile(path);
        if (fileInfoOpt.has_value()) {
            validFiles.append(fileInfoOpt.value());}}
    return validFiles;
}


std::optional<FileInfo> BatProcessor::probeFile(const QString& path){

    AVFormatContext* format_ctx_raw = nullptr;
    if (avformat_open_input(&format_ctx_raw, path.toUtf8().constData(), nullptr, nullptr) != 0) {
        return std::nullopt;}
    BatFFmpeg::BatFormatContextPtr formatContext(format_ctx_raw);

    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) {
        return std::nullopt;}

    int stream_index = av_find_best_stream(formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
        return std::nullopt;}

    AVStream* stream = formatContext->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        return std::nullopt;}

    FileInfo info;
    info.path = path;
    info.duration = (formatContext->duration == AV_NOPTS_VALUE) ? 0 : (formatContext->duration / 1000);
    info.codecName = codec->name;
    info.codecId = codec->id;

    return info;
}


void BatProcessor::startWorkers(QList<FileInfo>& allFiles, const BatSettings& settings){

    int threadCount = QThread::idealThreadCount();
    emit logMessage(BatUtils::getCurrentTimestamp() + QString(tr(" Initializing %1 worker thread(s).")).arg(threadCount));

    m_bucketIdPadding = (threadCount > 0) ? static_cast<int>(std::floor(std::log10(threadCount))) + 1 : 1;

    struct WeightedFileInfo {
        FileInfo info;
        qint64 weightedDuration;};

    QList<WeightedFileInfo> weightedFiles;
    for (const auto& file : allFiles) {
        int weight = CODEC_ID_WEIGHTS.value(file.codecId, DEFAULT_WEIGHT);
        weightedFiles.append({file, file.duration * weight});}

    std::sort(weightedFiles.begin(), weightedFiles.end(), [](const WeightedFileInfo& a, const WeightedFileInfo& b) {
        return a.weightedDuration > b.weightedDuration;});

    QList<Bucket> buckets(threadCount);
    QList<qint64> bucketWeightedDurations(threadCount, 0);

    for (const auto& weightedFile : weightedFiles) {
        auto minDurationIterator = std::min_element(bucketWeightedDurations.begin(), bucketWeightedDurations.end());
        int minIndex = std::distance(bucketWeightedDurations.begin(), minDurationIterator);
        buckets[minIndex].append(weightedFile.info);
        bucketWeightedDurations[minIndex] += weightedFile.weightedDuration;}

    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Assigning tasks..."));
    for (int i = 0; i < threadCount; ++i) {
        if (buckets[i].isEmpty()) continue;
        
        qint64 originalDuration = 0;
        for(const auto& file : buckets[i]) originalDuration += file.duration;

        emit logMessage(QString(tr("  [Task Group %1] Total Duration: %2s"))
            .arg(i + 1, m_bucketIdPadding, 10, QChar('0'))
            .arg(QString::number(originalDuration / 1000.0, 'f', 2)));
        
        for (const auto& file : buckets[i]) {
            emit logMessage(QString(tr("        [%1] %2"))
                .arg(file.codecName.toUpper())
                .arg(QFileInfo(file.path).fileName()));}}

    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Starting parallel processing..."));

    m_runningWorkers = threadCount;
    for (int i = 0; i < threadCount; ++i) {
        QThread* thread = new QThread(this);
        BatWorker* worker = new BatWorker(i + 1, m_bucketIdPadding, &m_stopFlag, &m_pauseFlag);
        worker->moveToThread(thread);

        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::started, this, [=, bucket = buckets[i]]() {
            QMetaObject::invokeMethod(worker, "processBucket", Qt::QueuedConnection, Q_ARG(Bucket, bucket), Q_ARG(BatSettings, settings));});

        connect(worker, &BatWorker::fileCompleted, this, &BatProcessor::onWorkerFileCompleted, Qt::QueuedConnection);
        connect(worker, &BatWorker::logMessage, this, &BatProcessor::onWorkerLog, Qt::QueuedConnection);
        connect(worker, &BatWorker::bucketFinished, this, &BatProcessor::onBucketFinished, Qt::QueuedConnection);
        connect(worker, &BatWorker::bucketFinished, thread, &QThread::quit);
        connect(thread, &QThread::finished, this, &BatProcessor::onWorkerFinished, Qt::QueuedConnection);
        connect(worker, &BatWorker::fileMetricsReported, this, &BatProcessor::onFileMetricsReported, Qt::QueuedConnection);

        m_workerThreads.append(thread);
        m_workers.append(worker);
        thread->start();}
}


void BatProcessor::pause() { m_pauseFlag = true; emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Task paused.")); emit batchPaused(); }


void BatProcessor::resume() { m_pauseFlag = false; emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Task resumed.")); emit batchResumed(); }


void BatProcessor::stop() { m_stopFlag = true; if (m_pauseFlag.load()) { resume(); } emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Terminating task, please wait...")); }


void BatProcessor::onWorkerFileCompleted(const QString& filePath, bool success, const QString& errorInfo){

    int current = ++m_processedFileCount;
    QString status = success ? "Successfully" : "Failed";
    QString reason = success ? "" : QString(tr("   Reason:  %1")).arg(errorInfo);
    emit logMessage(QString(tr("%1 [%2/%3] %4: %5%6"))
        .arg(BatUtils::getCurrentTimestamp())
        .arg(current)
        .arg(m_totalFileCount.load())
        .arg(status)
        .arg(QFileInfo(filePath).fileName())
        .arg(reason));
    
    if (!success) {
        std::lock_guard<std::mutex> locker(m_failedFilesMutex);
        m_failedFiles.append(filePath);}
    
    emit progressUpdated(current, m_totalFileCount.load());
}


void BatProcessor::onWorkerLog(const QString& message) {
    emit logMessage(message);
}


void BatProcessor::onBucketFinished(int bucketId, qint64 elapsedMs){
    std::lock_guard<std::mutex> lock(m_bucketDataMutex);
    m_bucketCompletionTimes[bucketId] = elapsedMs;
}


void BatProcessor::onFileMetricsReported(const FilePerformanceMetrics& metrics){

    emit logMessage(QString(tr("    Time Elapsed: Decoding %1ms, FFT %2ms, Rendering & Saving %3ms"))
        .arg(metrics.decodingTimeMs)
        .arg(metrics.fftTimeMs)
        .arg(metrics.renderingTimeMs));

    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_allFileMetrics.append(metrics);
}


void BatProcessor::onWorkerFinished(){

    if (--m_runningWorkers == 0) {
        qint64 elapsedMs = m_totalTaskTimer.elapsed();
        QString totalTimeStr = BatUtils::formatElapsedMs(elapsedMs);
        
        QString timestamp = BatUtils::getCurrentTimestamp();

        emit logMessage("----------------------------------------");
        
        {
            std::lock_guard<std::mutex> lock(m_bucketDataMutex);
            for (auto it = m_bucketCompletionTimes.constBegin(); it != m_bucketCompletionTimes.constEnd(); ++it) {
                QString timeStr = BatUtils::formatElapsedMs(it.value());
                emit logMessage(QString(tr("    Task Group %1 Time Elapsed: %2"))
                    .arg(it.key(), m_bucketIdPadding, 10, QChar('0'))
                    .arg(timeStr));}
        }
        
        emit logMessage(timestamp + tr(" Performance Summary"));

        qint64 totalDecode = 0, totalFft = 0, totalRender = 0;
        for(const auto& metrics : m_allFileMetrics) {
            totalDecode += metrics.decodingTimeMs;
            totalFft += metrics.fftTimeMs;
            totalRender += metrics.renderingTimeMs;}

        emit logMessage(QString(tr("    Total Decoding Time: %1s")).arg(totalDecode / 1000.0, 0, 'f', 2));
        emit logMessage(QString(tr("    Total FFT Time: %1s")).arg(totalFft / 1000.0, 0, 'f', 2));
        emit logMessage(QString(tr("    Total Rendering & Saving Time: %1s")).arg(totalRender / 1000.0, 0, 'f', 2));
        
        QString summary;
        if (m_stopFlag.load()) {
            summary = tr("Task terminated");}
            else {
                summary = tr("All tasks completed.\n");
                if (m_failedFiles.isEmpty()) {
                    summary += tr("All files processed successfully.");}
                    else {
                        summary += QString(tr("Total %1 file(s) failed:\n")).arg(m_failedFiles.size());
                        summary += m_failedFiles.join("\n");}}
        
        emit logMessage(BatUtils::getCurrentTimestamp() + tr(" Task finished."));
        emit logMessage(QString(tr("    Total Task Duration: %1")).arg(totalTimeStr));
        
        if (m_stopFlag.load()) {emit batchStopped();} 
        else {emit batchFinished(summary);}
        
        cleanup();
    }
}


void BatProcessor::cleanup(){

    for (QThread* thread : m_workerThreads) {
        if (thread->isRunning()) {
            thread->quit();
            thread->wait(5000);}}
    
    qDeleteAll(m_workerThreads);
    
    m_workers.clear();
    m_workerThreads.clear();
}