#include "BatchStreamIoScheduler.h"
#include "StorageTopologyAnalyzer.h"

#include <QObject> 

BatchStreamExecutionPlan BatchStreamIoScheduler::analyze(const QString& inputPath, const QString& outputPath, int threadCount) {
    BatchStreamExecutionPlan plan;
    VolumeTopology inTopo = StorageTopologyAnalyzer::analyze(inputPath);
    VolumeTopology outTopo = StorageTopologyAnalyzer::analyze(outputPath);
    bool inIsSSD = (inTopo.hasSSD && !inTopo.hasHDD && !inTopo.isNetworkStorage);
    bool outIsSSD = (outTopo.hasSSD && !outTopo.hasHDD && !outTopo.isNetworkStorage);
    bool isSameDisk = false;
    if (!inTopo.devices.isEmpty() && !outTopo.devices.isEmpty()) {
        for (auto it = inTopo.devices.constBegin(); it != inTopo.devices.constEnd(); ++it) {
            if (outTopo.devices.contains(it.key())) {
                isSameDisk = true;
                break;
            }
        }
    }
    if (!isSameDisk && !inTopo.volumePath.isEmpty() && !outTopo.volumePath.isEmpty()) {
        isSameDisk = (inTopo.volumePath.compare(outTopo.volumePath, Qt::CaseInsensitive) == 0);
    } else if (!isSameDisk) {
        QString inDrive = inputPath.mid(0, 3).toUpper();
        QString outDrive = outputPath.mid(0, 3).toUpper();
        isSameDisk = (inDrive == outDrive);
    }

    if (inIsSSD && outIsSSD) {
        plan.ioMode = BatchStreamIoThreadMode::None;
        plan.workerDirectWrite = true;
        plan.strategyName = QObject::tr("[检测] 输入路径和输出路径均为高速储存器");
    }
    else if (inIsSSD && !outIsSSD) {
        plan.ioMode = BatchStreamIoThreadMode::WriterOnly;
        plan.workerDirectWrite = false;
        plan.strategyName = QObject::tr("[检测] 输出路径为低速储存器");
    }
    else if (!inIsSSD && outIsSSD) {
        plan.ioMode = BatchStreamIoThreadMode::ReaderOnly;
        plan.workerDirectWrite = true;
        plan.strategyName = QObject::tr("[检测] 输入路径为低速储存器");
    }
    else if (isSameDisk) {
        plan.ioMode = BatchStreamIoThreadMode::Hybrid;
        plan.workerDirectWrite = false;
        plan.strategyName = QObject::tr("[检测] 输入和输出路径位于同一物理磁盘或卷");
    }
    else {
        if (threadCount >= 3) {
            plan.ioMode = BatchStreamIoThreadMode::SeparateReadWrite;
            plan.workerDirectWrite = false;
            plan.strategyName = QObject::tr("[检测] 输入和输出路径位于不同的低速储存器");
        } else {
            plan.ioMode = BatchStreamIoThreadMode::ReaderOnly;
            plan.workerDirectWrite = true;
            plan.strategyName = QObject::tr("[检测] 输入和输出为不同的低速储存器 但线程数不足");
        }
    }
    return plan;
}