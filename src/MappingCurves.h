#pragma once

#include <QCoreApplication>
#include <QString>
#include <QList>

enum class CurveType {
    XX,
    FM1, FM2, FM3, FM4, FM5, FM6, FM7, FM8, FM9,
    M1,  M2,  M3,  M4,  M5,  M6,  M7,  M8,  M9,
    D1,  D2,  D3,  D4,  D5,  D6,  D7,  D8,  D9,  D10,
    FD1, FD2, FD3, FD4, FD5, FD6, FD7, FD8, FD9, FD10,
    Z1,  Z2,  Z3,  Z4,  Z5,  Z6,  Z7,  Z8,
    FZ1, FZ2, FZ3, FZ4, FZ5, FZ6, FZ7, FZ8,
    Y1,  FY1,
    S1,  FS1,
    S2,  FS2,
    H1,  FH1,
    SX1, SX2
};

struct CurveInfo {
    CurveType type;
    QString id;
    QString displayText;
    bool hasSeparator;
};

class MappingCurves {
    Q_DECLARE_TR_FUNCTIONS(MappingCurves)

public:
    static double forward(double x, CurveType type, double maxFreq = 22050.0);
    static double inverse(double y, CurveType type, double maxFreq = 22050.0);
    static QList<CurveInfo> getAllCurves();
    static QString getName(CurveType type);
};