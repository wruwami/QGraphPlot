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

#include "QCoordinateTransform.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QLoggingCategory>

namespace qgraphplot
{

namespace
{
Q_LOGGING_CATEGORY(lcCoordinate, "qgraphplot.coordinate")
}

QCoordinateTransform::QCoordinateTransform(const QRectF& dataBounds,
                                           const QRectF& pixelRect,
                                           bool xLog,
                                           bool yLog)
    : m_dataBounds(dataBounds), m_pixelRect(pixelRect), m_xLog(xLog), m_yLog(yLog)
{
    if (m_dataBounds.width() <= 0.0 || m_dataBounds.height() <= 0.0 ||
        !qIsFinite(m_dataBounds.width()) || !qIsFinite(m_dataBounds.height()) ||
        !qIsFinite(m_dataBounds.left()) || !qIsFinite(m_dataBounds.top()) ||
        !qIsFinite(m_dataBounds.right()) || !qIsFinite(m_dataBounds.bottom())) {
        qCWarning(lcCoordinate) << "QCoordinateTransform: invalid or non-positive dataBounds"
                                   " dimensions or non-finite edges:"
                                << m_dataBounds;
        m_dataBounds = QRectF(0.0, 0.0, 1.0, 1.0);  // Fallback to unit square
    }

    // Validate Log boundaries (AI.md §3.3)
    if (m_xLog) {
        if (m_dataBounds.left() <= 0.0 || m_dataBounds.right() <= 0.0) {
            qCWarning(lcCoordinate) << "QCoordinateTransform: X log scale requires positive "
                                       "bounds. Falling back to linear.";
            m_xLog = false;
        } else {
            m_logMinX = std::log10(m_dataBounds.left());
            m_logMaxX = std::log10(m_dataBounds.right());
        }
    }
    if (m_yLog) {
        // In dataBounds (QRectF), top() is minY, bottom() is maxY.
        if (m_dataBounds.top() <= 0.0 || m_dataBounds.bottom() <= 0.0) {
            qCWarning(lcCoordinate) << "QCoordinateTransform: Y log scale requires positive "
                                       "bounds. Falling back to linear.";
            m_yLog = false;
        } else {
            m_logMinY = std::log10(m_dataBounds.top());
            m_logMaxY = std::log10(m_dataBounds.bottom());
        }
    }
}

QPointF QCoordinateTransform::toPixel(const QPointF& data) const noexcept
{
    double px = m_pixelRect.left();
    double py = m_pixelRect.bottom();

    // X transformation
    if (m_xLog) {
        if (m_logMaxX > m_logMinX) {
            const double val = std::log10(std::max(data.x(), m_dataBounds.left()));
            const double pct = (val - m_logMinX) / (m_logMaxX - m_logMinX);
            px = m_pixelRect.left() + pct * m_pixelRect.width();
        } else {
            px = m_pixelRect.left() + m_pixelRect.width() / 2.0;
        }
    } else {
        if (m_dataBounds.width() > 0.0) {
            const double pct = (data.x() - m_dataBounds.left()) / m_dataBounds.width();
            px = m_pixelRect.left() + pct * m_pixelRect.width();
        } else {
            px = m_pixelRect.left() + m_pixelRect.width() / 2.0;
        }
    }

    // Y transformation (screen y increases downwards, so we subtract from bottom)
    if (m_yLog) {
        if (m_logMaxY > m_logMinY) {
            const double val = std::log10(std::max(data.y(), m_dataBounds.top()));
            const double pct = (val - m_logMinY) / (m_logMaxY - m_logMinY);
            py = m_pixelRect.bottom() - pct * m_pixelRect.height();
        } else {
            py = m_pixelRect.bottom() - m_pixelRect.height() / 2.0;
        }
    } else {
        if (m_dataBounds.height() > 0.0) {
            const double pct = (data.y() - m_dataBounds.top()) / m_dataBounds.height();
            py = m_pixelRect.bottom() - pct * m_pixelRect.height();
        } else {
            py = m_pixelRect.bottom() - m_pixelRect.height() / 2.0;
        }
    }

    return QPointF(px, py);
}

QPointF QCoordinateTransform::toData(const QPointF& pixel) const noexcept
{
    double dx = m_dataBounds.left();
    double dy = m_dataBounds.top();

    // X inverse transformation
    if (m_xLog) {
        if (m_pixelRect.width() > 0.0) {
            const double pct = (pixel.x() - m_pixelRect.left()) / m_pixelRect.width();
            const double logX = m_logMinX + pct * (m_logMaxX - m_logMinX);
            dx = std::pow(10.0, logX);
        }
    } else {
        if (m_pixelRect.width() > 0.0) {
            const double pct = (pixel.x() - m_pixelRect.left()) / m_pixelRect.width();
            dx = m_dataBounds.left() + pct * m_dataBounds.width();
        }
    }

    // Y inverse transformation (screen y increases downwards)
    if (m_yLog) {
        if (m_pixelRect.height() > 0.0) {
            const double pct = (m_pixelRect.bottom() - pixel.y()) / m_pixelRect.height();
            const double logY = m_logMinY + pct * (m_logMaxY - m_logMinY);
            dy = std::pow(10.0, logY);
        }
    } else {
        if (m_pixelRect.height() > 0.0) {
            const double pct = (m_pixelRect.bottom() - pixel.y()) / m_pixelRect.height();
            dy = m_dataBounds.top() + pct * m_dataBounds.height();
        }
    }

    return QPointF(dx, dy);
}

}  // namespace qgraphplot
