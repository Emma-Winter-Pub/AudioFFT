#pragma once

#include "ISpectrumRenderer.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class SpectrumRendererGpu : public QOpenGLWidget, protected QOpenGLFunctions, public ISpectrumRenderer {
    Q_OBJECT

public:
    explicit SpectrumRendererGpu(QWidget* parent = nullptr);
    ~SpectrumRendererGpu() override;
    void setData(const std::vector<float>& data) override;
    void clearData() override;
    void setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type) override;
    void setDrawRect(const QRect& rect) override;
    QWidget* getWidget() override { return this; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    std::vector<float> m_dataCache; 
    std::vector<float> m_vertexBufferData;
    QColor m_lineColor = Qt::yellow;
    int m_lineWidth = 1;
    bool m_isFilled = false;
    int m_fillAlpha = 50;
    SpectrumProfileType m_type = SpectrumProfileType::Line;
    QRect m_drawRect;
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    bool m_dataDirty = false;
    int m_totalVertexCount = 0;
    int m_fillVertexCount = 0;
    int m_lineVertexCount = 0;
    int m_lineStartOffset = 0;
    void updateVertexBuffer();
};