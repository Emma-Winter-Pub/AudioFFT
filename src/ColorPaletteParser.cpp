#include "ColorPaletteParser.h"
#include "ColorPaletteEntity.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>
#include <vector>
#include <array>
#include <algorithm>

struct ColorNode {
    double pos;
    int r, g, b;
};

std::shared_ptr<XColorPalette> ColorPaletteParser::parse(const QString& filepath, QString& outError) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        outError = QString("无法打开文件：%1").arg(filepath);
        return nullptr;
    }

    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif

    QStringList lines;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith("//")) {
            lines.append(line);
        }
    }
    file.close();

    if (lines.size() < 3) {
        outError = "数据行数不足，文件格式损坏。";
        return nullptr;
    }
    QString name = lines[0];
    QString mode = lines[1].toLower();
    QString id = QFileInfo(filepath).completeBaseName();
    std::array<uint32_t, 256> colors;
    if (mode == "scientific") {
        if (lines.size() - 2 < 256) {
            outError = QString("scientific 模式需要恰好 256 行色彩代码，当前仅有 %1 行。").arg(lines.size() - 2);
            return nullptr;
        }
        QRegularExpression hexRegex("^[0-9a-fA-F]{6}$");
        for (int i = 0; i < 256; ++i) {
            QString hexStr = lines[i + 2];
            if (hexStr.startsWith("#")) hexStr = hexStr.mid(1);

            if (!hexRegex.match(hexStr).hasMatch()) {
                outError = QString("第 %1 行的 16 进制颜色格式错误：%2").arg(i + 3).arg(hexStr);
                return nullptr;
            }
            bool ok;
            uint32_t val = hexStr.toUInt(&ok, 16);
            if (!ok) {
                outError = QString("第 %1 行的颜色解析失败。").arg(i + 3);
                return nullptr;
            }
            colors[i] = 0xFF000000 | val;
        }
    }
    else if (mode == "linear") {
        std::vector<ColorNode> nodes;
        for (int i = 2; i < lines.size(); ++i) {
            QStringList parts = lines[i].split(',');
            if (parts.size() != 4) {
                outError = QString("第 %1 行节点格式错误，期望(pos,R,G,B)。").arg(i + 1);
                return nullptr;
            }
            bool okP, okR, okG, okB;
            double pos = parts[0].toDouble(&okP);
            int r = parts[1].toInt(&okR);
            int g = parts[2].toInt(&okG);
            int b = parts[3].toInt(&okB);
            if (!okP || !okR || !okG || !okB) {
                outError = QString("第 %1 行数值解析失败。").arg(i + 1);
                return nullptr;
            }
            if (pos < 0.0 || pos > 1.0 || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                outError = QString("第 %1 行数值越界（pos: 0~1，RGB: 0~255）。").arg(i + 1);
                return nullptr;
            }
            nodes.push_back({ pos, r, g, b });
        }
        if (nodes.empty()) {
            outError = "linear 模式下没有找到有效的色彩节点。";
            return nullptr;
        }
        std::sort(nodes.begin(), nodes.end(), [](const ColorNode& a, const ColorNode& b) {
            return a.pos < b.pos;
        });
        for (int i = 0; i < 256; ++i) {
            double pos = i / 255.0;
            ColorNode p1 = nodes.front();
            ColorNode p2 = nodes.back();
            for (size_t j = 0; j < nodes.size() - 1; ++j) {
                if (pos >= nodes[j].pos && pos <= nodes[j+1].pos) {
                    p1 = nodes[j];
                    p2 = nodes[j+1];
                    break;
                }
            }
            double t = (p2.pos > p1.pos) ? (pos - p1.pos) / (p2.pos - p1.pos) : 0.0;
            int r = static_cast<int>(p1.r + t * (p2.r - p1.r));
            int g = static_cast<int>(p1.g + t * (p2.g - p1.g));
            int b = static_cast<int>(p1.b + t * (p2.b - p1.b));
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            colors[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    else {
        outError = QString("不支持的色彩模式标志：'%1'。期望 'scientific' 或 'linear'。").arg(mode);
        return nullptr;
    }
    return std::make_shared<ColorPaletteEntity>(id, name, colors);
}