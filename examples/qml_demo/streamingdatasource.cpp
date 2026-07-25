#include "streamingdatasource.h"

#include <cmath>
#include <vector>

StreamingDataSource::StreamingDataSource(QObject* parent) : QObject(parent) {}

void StreamingDataSource::generateFrame()
{
    std::vector<QPointF> batch;
    batch.reserve(static_cast<size_t>(kPointsPerFrame));
    for (int i = 0; i < kPointsPerFrame; ++i) {
        m_xMax += kSampleDt;
        const double y = std::sin(kTwoPi * kSignalFrequencyHz * m_xMax);
        batch.emplace_back(m_xMax, y);
    }
    m_model.appendRange(batch);
    emit windowChanged();
}
