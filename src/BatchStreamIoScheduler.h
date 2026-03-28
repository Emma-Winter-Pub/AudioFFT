#pragma once

#include <QString>

enum class BatchStreamIoThreadMode {
    None,
    ReaderOnly,
    WriterOnly,
    SeparateReadWrite,
    Hybrid
};

struct BatchStreamExecutionPlan {
    bool workerDirectWrite = true;
    BatchStreamIoThreadMode ioMode = BatchStreamIoThreadMode::None;
    QString strategyName;
    QString strategyDescription;
};

class BatchStreamIoScheduler {
public:
    static BatchStreamExecutionPlan analyze(const QString& inputPath, const QString& outputPath, int threadCount);
};