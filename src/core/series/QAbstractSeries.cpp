// QGraphPlot — High-performance Qt chart library
//
// Copyright 2026 QGraphPlot Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! @file QAbstractSeries.cpp
//! @brief QAbstractSeries implementation.

#include "QAbstractSeries.h"

#include <cmath>
#include <utility>

namespace qgraphplot
{

QAbstractSeries::QAbstractSeries(QObject* parent) : QObject(parent) {}

QAbstractSeries::~QAbstractSeries() = default;

// ─────────────────────────────────────────────────────────────
// Property setters — each emits only when the value actually
// changes (prevents spurious signal cascades on the render thread).
// ─────────────────────────────────────────────────────────────

void QAbstractSeries::setModel(QAbstractSeriesModel* model)
{
    if (m_model == model) {
        return;
    }
    if (m_modelDestroyedConnection) {
        disconnect(m_modelDestroyedConnection);
    }
    m_model = model;
    if (m_model) {
        m_modelDestroyedConnection = connect(m_model, &QObject::destroyed, this, [this]() {
            m_model = nullptr;
            m_modelDestroyedConnection = QMetaObject::Connection();
            Q_EMIT modelChanged(nullptr);
        });
    } else {
        m_modelDestroyedConnection = QMetaObject::Connection();
    }
    Q_EMIT modelChanged(m_model);
}

void QAbstractSeries::setColor(QColor color)
{
    if (m_color == color) {
        return;
    }
    m_color = std::move(color);
    Q_EMIT colorChanged(m_color);
}

void QAbstractSeries::setName(QString name)
{
    if (m_name == name) {
        return;
    }
    m_name = std::move(name);
    Q_EMIT nameChanged(m_name);
}

void QAbstractSeries::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }
    m_visible = visible;
    Q_EMIT visibleChanged(m_visible);
}

void QAbstractSeries::setLineWidth(double lineWidth)
{
    if (!std::isfinite(lineWidth) || lineWidth <= 0.0) {
        qWarning("QAbstractSeries::setLineWidth: lineWidth must be finite and > 0");
        return;
    }
    if (qFuzzyCompare(m_lineWidth, lineWidth)) {
        return;
    }
    m_lineWidth = lineWidth;
    Q_EMIT lineWidthChanged(m_lineWidth);
}

void QAbstractSeries::setDashPattern(const QList<qreal>& dashPattern)
{
    if (!isValidDashPattern(dashPattern)) {
        qWarning("QAbstractSeries::setDashPattern: pattern must have an even number of finite, "
                 "positive entries");
        return;
    }
    if (m_dashPattern == dashPattern) {
        return;
    }
    m_dashPattern = dashPattern;
    Q_EMIT dashPatternChanged();
}

bool QAbstractSeries::isValidDashPattern(const QList<qreal>& dashPattern)
{
    if (dashPattern.isEmpty()) {
        return true;  // Solid line.
    }
    if (dashPattern.size() % 2 != 0) {
        return false;
    }
    for (const qreal length : dashPattern) {
        if (!std::isfinite(length) || length <= 0.0) {
            return false;
        }
    }
    return true;
}

}  // namespace qgraphplot
