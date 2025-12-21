#pragma once

#include <QObject>
#include <QString>
#include <vector>


class FftProcessor : public QObject {
    Q_OBJECT

public:
    explicit FftProcessor(QObject *parent = nullptr);

    bool compute(const std::vector<float>& pcmData,
                 std::vector<std::vector<double>>& spectrogramData,
                 double timeInterval,
                 int fftSize,
                 int nativeSampleRate);

signals:
    void logMessage(const QString& message);

};