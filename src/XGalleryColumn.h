#pragma once

#include "WindowGalleryTypes.h"
#include <QString>
#include <QMenu>
#include <QSettings>

class XGalleryColumn {
public:
    virtual ~XGalleryColumn() = default;
    virtual GalleryColumnType type() const = 0;
    virtual QString headerText() const = 0;
    virtual int defaultAlignment() const = 0;
    virtual bool hasConfigMenu() const = 0;
    virtual QMenu* createConfigMenu(QWidget* parent) = 0;
    virtual QString formatDisplay(const GalleryItemData& item) const = 0;
    virtual void loadConfig(QSettings& settings) = 0;
    virtual void saveConfig(QSettings& settings) const = 0;
};