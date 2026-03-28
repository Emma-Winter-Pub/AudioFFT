#include "BatchStreamIoScheduler.h"
#include "StorageUtils.h"

#include <QObject> 

BatchStreamExecutionPlan BatchStreamIoScheduler::analyze(const QString& inputPath, const QString& outputPath, int threadCount) {
    BatchStreamExecutionPlan plan;
    StorageType inType = StorageUtils::detectStorageType(inputPath);
    StorageType outType = StorageUtils::detectStorageType(outputPath);
    bool inIsSSD = (inType == StorageType::SSD);
    bool outIsSSD = (outType == StorageType::SSD);
    int inDiskID = StorageUtils::getPhysicalDiskID(inputPath);
    int outDiskID = StorageUtils::getPhysicalDiskID(outputPath);
    bool isSameDisk = false;
    if (inDiskID != -1 && outDiskID != -1) {
        isSameDisk = (inDiskID == outDiskID);
    } else {
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
        plan.strategyName = QObject::tr("[检测] 输入路径和输出路径为同一个低速储存器");
    }
    else {
        if (threadCount >= 3) {
            plan.ioMode = BatchStreamIoThreadMode::SeparateReadWrite;
            plan.workerDirectWrite = false;
            plan.strategyName = QObject::tr("[检测] 输入路径和输出路径为不同的低速储存器");
        } else {
            plan.ioMode = BatchStreamIoThreadMode::ReaderOnly;
            plan.workerDirectWrite = true;
            plan.strategyName = QObject::tr("[检测] 输入路径和输出路径为不同的低速储存器 线程数不足");
        }
    }
    return plan;
}