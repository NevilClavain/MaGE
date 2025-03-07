/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#define _USE_MATH_DEFINES

#include <windows.h>
#include <cmath>
#include <numbers>

#include "timemanager.h"
//#include "syncvariable.h"

using namespace mage::core;


/////////////////////////////////////////////////

void TimerDescr::setState(bool p_state)
{
    m_state = p_state;
    m_prev_tick = -1;
    m_tick_count = 0;
    m_freeze = false;
}

void TimerDescr::suspend(bool p_suspend)
{
    m_freeze = p_suspend;
    if (p_suspend)
    {
        m_prev_tick = -1;
    }
}

void TimerDescr::setPeriod(long p_period)
{
    m_period = p_period;
}

void TimerDescr::expired(void)
{
    for (const auto& call : m_callbacks)
    {
        call(TimerEvents::TIMER_EXPIRED);
    }
}

/////////////////////////////////////////////////

void TimeManager::reset(void)
{
    m_last_tick = 0;
    m_current_tick = 0;
    m_fps = 0;
    m_frame_count = 0;
    m_ready = false;
    m_last_deltatime = 0;
}

void TimeManager::update(void)
{
    const auto current_tick{ ::GetTickCount() };

    if (m_last_tick)
    {
        if (current_tick - m_last_tick >= 1000)
        {
            m_last_deltatime = current_tick - m_last_tick;
            m_last_tick = current_tick;
            m_fps = m_frame_count;
            m_frame_count = 0;

            if (m_fps > 1)
            {
                // declare ready only if we get a decent fps
                m_ready = true;
            }
        }
    }
    else
    {
        m_last_tick = current_tick;
    }
    m_frame_count++;

    if (m_ready)
    {
        // timers management
        for (auto it = m_timers.begin(); it != m_timers.end(); ++it)
        {
            TimerDescr* timer{ (*it) };

            if (timer->m_state && !timer->m_freeze)
            {
                if (-1 == timer->m_prev_tick)
                {
                    timer->m_prev_tick = current_tick;
                }
                else
                {
                    timer->m_tick_count += current_tick - timer->m_prev_tick;
                    if (timer->m_tick_count >= timer->m_period)
                    {
                        timer->m_tick_count = 0;
                        timer->expired();
                    }

                    timer->m_prev_tick = current_tick;
                }
            }
        }
        m_current_tick = current_tick;
    }
}


bool TimeManager::isReady(void) const
{
    return m_ready;
}

long TimeManager::getLastDeltaTime(void) const
{
    return m_last_deltatime;
}

long TimeManager::getCurrentTick(void) const
{
    return m_current_tick;
}

long TimeManager::getFPS(void) const
{
    return m_fps;
}

double TimeManager::convertUnitPerSecFramePerSec(double p_speed)
{
    if (!m_ready)
    {
        return 0.0;
    }
    return (p_speed / m_fps);
}

void TimeManager::angleSpeedInc(double* p_angle, double p_angleSpeed)
{
    if (!m_ready) return;

    // on veut, a partir de la vitesse en degres/s fixee, trouver
    // la vitesse en degres / frame -> on fait donc (deg/sec)/(frame/sec) 
    const double angleSpeedDegPerFrame{ p_angleSpeed / m_fps };
    double angle{ *p_angle };

    angle += angleSpeedDegPerFrame;
    angle = std::fmod(angle, 2 * M_PI);

    *p_angle = angle;
}

void TimeManager::angleSpeedDec(double* p_angle, double p_angleSpeed)
{
    if (!m_ready) return;

    // on veut, a partir de la vitesse en degres/s fixee, trouver
    // la vitesse en degres / frame -> on fait donc (deg/sec)/(frame/sec) 
    const double angleSpeedDegPerFrame{ p_angleSpeed / m_fps };
    double angle{ *p_angle };

    angle -= angleSpeedDegPerFrame;
    angle = std::fmod(angle, 2 * M_PI);

    if (*p_angle <= 0.0f)
    {
        angle = 2 * M_PI + angle;
    }

    *p_angle = angle;
}

void TimeManager::translationSpeedInc(double* p_translation, double p_speed)
{
    if (!m_ready) return;

    // on veut, a partir de la vitesse en unites/s fixee, trouver
    // la vitesse en unite / frame -> on fait donc (unit/sec)/(frame/sec)
    const double translationSpeedUnitPerFrame{ p_speed / m_fps };
    *p_translation += translationSpeedUnitPerFrame;
}

void TimeManager::translationSpeedDec(double* p_translation, double p_speed)
{
    if (!m_ready) return;

    // on veut, a partir de la vitesse en unites/s fixee, trouver
    // la vitesse en unite / frame -> on fait donc (unit/sec)/(frame/sec)
    const double translationSpeedUnitPerFrame{ p_speed / m_fps };
    *p_translation -= translationSpeedUnitPerFrame;
}


void TimeManager::registerTimer(TimerDescr* p_timer)
{
    m_timers.insert(p_timer);
}


