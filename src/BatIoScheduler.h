#pragma once

#include <QString>

enum class BatIoThreadMode {
    None,
    ReaderOnly,
    WriterOnly,
    SeparateReadWrite,
    Hybrid
};

struct BatExecutionPlan {
    bool workerDirectWrite = true;
    BatIoThreadMode ioMode = BatIoThreadMode::None;
    QString strategyName;
    QString strategyDescription;
};

class BatIoScheduler {
public:
    static BatExecutionPlan analyze(const QString& inputPath, const QString& outputPath, int threadCount);
};