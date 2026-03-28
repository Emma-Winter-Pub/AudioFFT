#include "SpectrumRendererGpu.h"

#include <QMatrix4x4>
#include <QSurfaceFormat>

SpectrumRendererGpu::SpectrumRendererGpu(QWidget* parent)
    : QOpenGLWidget(parent) 
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setFormat(QSurfaceFormat::defaultFormat());
}

SpectrumRendererGpu::~SpectrumRendererGpu() {
    makeCurrent();
    if (m_vbo.isCreated()) m_vbo.destroy();
    if (m_vao.isCreated()) m_vao.destroy();
    delete m_program;
    doneCurrent();
}

void SpectrumRendererGpu::setData(const std::vector<float>& data) {
    m_dataCache = data;
    m_dataDirty = true;
    update();
}

void SpectrumRendererGpu::clearData() {
    if (!m_dataCache.empty()) {
        m_dataCache.clear();
        m_dataDirty = true;
        update();
    }
}

void SpectrumRendererGpu::setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type) {
    if (m_lineColor != color || m_lineWidth != lineWidth || 
        m_isFilled != filled || m_fillAlpha != fillAlpha || m_type != type) 
    {
        m_lineColor = color;
        m_lineWidth = lineWidth;
        m_isFilled = filled;
        m_fillAlpha = fillAlpha;
        m_type = type;
        m_dataDirty = true;
        update();
    }
}

void SpectrumRendererGpu::setDrawRect(const QRect& rect) {
    if (m_drawRect != rect) {
        m_drawRect = rect;
        update();
    }
}

void SpectrumRendererGpu::initializeGL() {
    initializeOpenGLFunctions();
    m_program = new QOpenGLShaderProgram();
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/overlay.vert")) {}
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/overlay.frag")) {}
    m_program->link();
    m_vao.create();
    m_vao.bind();
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(nullptr, 65536 * sizeof(float) * 4);
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));
    m_vao.release();
    m_vbo.release();
}

void SpectrumRendererGpu::updateVertexBuffer() {
    if (m_dataCache.empty()) {
        m_totalVertexCount = 0;
        return;
    }
    size_t count = m_dataCache.size();
    m_vertexBufferData.clear();
    m_fillVertexCount = 0;
    m_lineVertexCount = 0;
    m_lineStartOffset = 0;
    if (m_type == SpectrumProfileType::Bar) {
        m_vertexBufferData.reserve(count * 8); 
        for (size_t i = 0; i < count; ++i) {
            float x = static_cast<float>(i) / (count - 1);
            float y = m_dataCache[i];
            if (y > 1.0f) y = 1.0f;
            if (y < 0.0f) y = 0.0f;
            m_vertexBufferData.push_back(x); m_vertexBufferData.push_back(0.0f);
            m_vertexBufferData.push_back(x); m_vertexBufferData.push_back(y);
        }
        m_fillVertexCount = static_cast<int>(count * 2);
        m_lineStartOffset = m_fillVertexCount;
        float halfWidth = 0.0f;
        if (count > 1) {
            halfWidth = (1.0f / (count - 1)) * 0.5f;
        }
        for (size_t i = 0; i < count; ++i) {
            float x = static_cast<float>(i) / (count - 1);
            float y = m_dataCache[i];
            if (y > 1.0f) y = 1.0f;
            if (y < 0.0f) y = 0.0f;
            m_vertexBufferData.push_back(x - halfWidth); m_vertexBufferData.push_back(y);
            m_vertexBufferData.push_back(x + halfWidth); m_vertexBufferData.push_back(y);
        }
        m_lineVertexCount = static_cast<int>(count * 2);
        m_totalVertexCount = m_fillVertexCount + m_lineVertexCount;
    } 
    else {
        if (m_isFilled) {
            m_vertexBufferData.reserve(count * 4);
            for (size_t i = 0; i < count; ++i) {
                float x = static_cast<float>(i) / (count - 1);
                float y = m_dataCache[i];
                if (y > 1.0f) y = 1.0f; 
                if (y < 0.0f) y = 0.0f;
                m_vertexBufferData.push_back(x); m_vertexBufferData.push_back(y);
                m_vertexBufferData.push_back(x); m_vertexBufferData.push_back(0.0f);
            }
            m_fillVertexCount = static_cast<int>(count * 2);
        }
        m_lineStartOffset = static_cast<int>(m_vertexBufferData.size() / 2);
        for (size_t i = 0; i < count; ++i) {
            float x = static_cast<float>(i) / (count - 1);
            float y = m_dataCache[i];
            if (y > 1.0f) y = 1.0f; 
            if (y < 0.0f) y = 0.0f;
            m_vertexBufferData.push_back(x); m_vertexBufferData.push_back(y);
        }
        m_lineVertexCount = static_cast<int>(count);
        m_totalVertexCount = m_fillVertexCount + m_lineVertexCount;
    }
    m_vbo.bind();
    size_t dataSize = m_vertexBufferData.size() * sizeof(float);
    if (dataSize > static_cast<size_t>(m_vbo.size())) {
        m_vbo.allocate(m_vertexBufferData.data(), static_cast<int>(dataSize));
    } else {
        m_vbo.write(0, m_vertexBufferData.data(), static_cast<int>(dataSize));
    }
    m_vbo.release();
}

void SpectrumRendererGpu::resizeGL(int w, int h) {
    Q_UNUSED(w); Q_UNUSED(h);
}

void SpectrumRendererGpu::paintGL() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (m_dataCache.empty() || !m_drawRect.isValid() || m_drawRect.isEmpty()) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_vao.bind();
    m_program->bind();
    if (m_dataDirty) {
        updateVertexBuffer();
        m_dataDirty = false;
    }
    if (m_totalVertexCount > 0) {
        QMatrix4x4 mvp;
        mvp.ortho(0, width(), height(), 0, -1, 1); 
        float rectX = static_cast<float>(m_drawRect.x());
        float rectY = static_cast<float>(m_drawRect.y());
        float rectW = static_cast<float>(m_drawRect.width());
        float rectH = static_cast<float>(m_drawRect.height());
        mvp.translate(rectX, rectY, 0.0f);
        mvp.translate(0.0f, rectH, 0.0f);   
        mvp.scale(rectW, -rectH, 1.0f);     
        m_program->setUniformValue("mvp", mvp);
        if (m_type == SpectrumProfileType::Bar) {
            if (m_isFilled) {
                QColor fillColor = m_lineColor;
                fillColor.setAlpha(m_fillAlpha);
                m_program->setUniformValue("uColor", fillColor);
                glLineWidth(std::max(1.0f, static_cast<GLfloat>(m_lineWidth) + 2.0f));
                glDrawArrays(GL_LINES, 0, m_fillVertexCount);
            }
            QColor lineColor = m_lineColor;
            lineColor.setAlpha(255);
            m_program->setUniformValue("uColor", lineColor);
            glLineWidth(static_cast<GLfloat>(m_lineWidth));
            glDrawArrays(GL_LINES, m_lineStartOffset, m_lineVertexCount);
        } 
        else {
            if (m_isFilled && m_fillVertexCount > 0) {
                QColor fillColor = m_lineColor;
                fillColor.setAlpha(m_fillAlpha); 
                m_program->setUniformValue("uColor", fillColor);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, m_fillVertexCount);
            }
            QColor lineColor = m_lineColor;
            lineColor.setAlpha(255); 
            m_program->setUniformValue("uColor", lineColor);
            glLineWidth(static_cast<GLfloat>(m_lineWidth));
            glDrawArrays(GL_LINE_STRIP, m_lineStartOffset, m_lineVertexCount);
        }
    }
    m_program->release();
    m_vao.release();
}