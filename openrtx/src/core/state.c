/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <ui.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <event.h>
#include <state.h>
#include <battery.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>

state_t state;
pthread_mutex_t state_mutex;
static long long int lastUpdate = 0;

// Commonly used frequency steps, expressed in Hz
const uint32_t freq_steps[] = { 1000, 5000, 6250, 10000, 12500, 15000,
                                20000, 25000, 50000, 100000 };
const size_t n_freq_steps   = sizeof(freq_steps) / sizeof(freq_steps[0]);

static bool volume_changed;
static uint8_t prev_volume;

void state_init()
{
    pthread_mutex_init(&state_mutex, NULL);

    /*
     * Try loading settings from nonvolatile memory and default to sane values
     * in case of failure.
     */
    if(nvm_readSettings(&state.settings) < 0)
    {
        state.settings = default_settings;
        strncpy(state.settings.callsign, "OPNRTX", 10);
    }

    /*
     * Try loading VFO configuration from nonvolatile memory and default to sane
     * values in case of failure.
     */
    if(nvm_readVfoChannelData(&state.channel) < 0)
    {
        state.channel = cps_getDefaultChannel();
    }

    /*
     * Initialise remaining fields
     */
    #ifdef CONFIG_RTC
    state.time = platform_getCurrentTime();
    #endif
    state.v_bat  = platform_getVbat();
    state.charge = battery_getCharge(state.v_bat);
    state.rssi   = -127.0f;
    state.volume = platform_getVolumeLevel();
    state.volume_changed = false;
    state.channel_index = 0;    // Set default channel index (it is 0-based)
    state.bank_enabled  = false;
    state.rtxStatus     = RTX_OFF;
    state.emergency     = false;
    state.txDisable     = false;
    state.pauseNotifications = false;
    state.step_index    = 4; // Default frequency step 12.5kHz

    prev_volume = state.volume;

    // Force brightness field to be in range 0 - 100
    if(state.settings.brightness > 100)
    {
        state.settings.brightness = 100;
    }
}

void state_terminate()
{
    // Never store a brightness of 0 to avoid booting with a black screen
    if(state.settings.brightness == 0)
    {
        state.settings.brightness = 5;
    }

    nvm_writeSettingsAndVfo(&state.settings, &state.channel);
    pthread_mutex_destroy(&state_mutex);
}

void state_task()
{
    // Update radio state once every 100ms
    if((getTick() - lastUpdate) < 100)
        return;

    lastUpdate = getTick();

    pthread_mutex_lock(&state_mutex);

    /*
     * Low-pass filtering with a time constant of 10s when updated at 1Hz
     * Original computation: state.v_bat = 0.02*vbat + 0.98*state.v_bat
     * Peak error is 18mV when input voltage is 49mV.
     *
     * NOTE: GD77 and DM-1801 already have an hardware low-pass filter on the
     * vbat pin. Adding also the digital one seems to cause more troubles than
     * benefits.
     */
    uint16_t vbat = platform_getVbat();
    #if defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
    state.v_bat   = vbat;
    #else
    state.v_bat  -= (state.v_bat * 2) / 100;
    state.v_bat  += (vbat * 2) / 100;
    #endif

    /*
     * Update volume level, as a 50% average between previous value and a new
     * read of the knob position. This gives a good reactivity while preventing
     * the volume level to jitter when the knob is not being moved.
     */
    uint16_t vol = platform_getVolumeLevel() + state.volume;
    state.volume = vol / 2;

    if(state.volume != prev_volume) {
        prev_volume = state.volume;
        state.volume_changed = true;
    } 
    
    state.charge = battery_getCharge(state.v_bat);
    state.rssi = rtx_getRssi();

    #ifdef CONFIG_RTC
    state.time = platform_getCurrentTime();
    #endif

    pthread_mutex_unlock(&state_mutex);

    ui_pushEvent(EVENT_STATUS, 0);
}

void state_resetSettingsAndVfo()
{
    state.settings = default_settings;
    state.channel  = cps_getDefaultChannel();
}
