#pragma once

#include <QString>
#include <QByteArray>

class TextEncodingHelper {
public:
    static QString decode(const QByteArray& data);

private:
    static int scoreGB18030(const QByteArray& data);
    static int scoreShiftJIS(const QByteArray& data);
    static int scoreBig5(const QByteArray& data);
};