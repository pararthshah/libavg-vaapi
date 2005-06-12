//
// $Id$
//

#include "ProfilingZone.h"
#include "Profiler.h"

#include <iostream>

using namespace std;

namespace avg {

ProfilingZone::ProfilingZone(const string& sName)
    : m_sName(sName),
      m_TimeSum(0),
      m_AvgTime(0),
      m_NumFrames(0),
      m_bIsRegistered(false)
{
}

ProfilingZone::~ProfilingZone() 
{
}

void ProfilingZone::clear()
{
    m_TimeSum = 0;
    m_AvgTime = 0;
    m_NumFrames = 0;
    m_bIsRegistered = false;
}

void ProfilingZone::start()
{
    // Start gets called when the zone is first entered. 
    if (!m_bIsRegistered && Profiler::get().isRunning()) {
        // This stuff makes sure that the zones are registered in the order 
        // they are entered.
        Profiler::get().addZone(*this);
        clear();
        m_bIsRegistered = true;
    }
    Profiler::get().setActiveZone(this);
}

void ProfilingZone::reset()
{
    m_NumFrames++;
    m_AvgTime = (m_AvgTime*(m_NumFrames-1)+m_TimeSum)/m_NumFrames;
    m_TimeSum = 0;
}

CycleCount ProfilingZone::getUSecs() const
{
    return (m_TimeSum*1000*1000)/TimeSource::get()->getCyclesPerSecond();
}

CycleCount ProfilingZone::getAvgUSecs() const
{
    return (m_AvgTime*1000*1000)/TimeSource::get()->getCyclesPerSecond();
}

const std::string& ProfilingZone::getName() const
{
    return m_sName;
}


void ProfilingZone::addCycles(CycleCount Cycles)
{
    m_TimeSum += Cycles;
}

}
