#pragma once

#include <QString>
#include <functional>

class ColorUserPaletteLoader {
public:
    static void loadUserPalettes(std::function<void(const QString&)> logger = nullptr);

private:
    static void createReadmeIfNotExist(const QString& dirPath);
};