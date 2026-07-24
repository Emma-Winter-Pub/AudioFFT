#pragma once

#include <QString>

enum class BatchIoThreadMode {
    None,
    ReaderOnly,
    WriterOnly,
    SeparateReadWrite,
    Hybrid
};

struct BatchExecutionPlan {
    bool workerDirectWrite = true;
    BatchIoThreadMode ioMode = BatchIoThreadMode::None;
    QString strategyName;
    QString strategyDescription;
};

class BatchFullLoadIoScheduler {
public:
    static BatchExecutionPlan analyze(const QString& inputPath, const QString& outputPath, int threadCount);
};