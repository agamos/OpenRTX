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

#include <interfaces/platform.h>
#include <interfaces/cps_io.h>
#include <stdio.h>
#include <stdint.h>
#include <ui/ui_default.h>
#include <string.h>
#include <ui/ui_strings.h>
#include <utils.h>

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(CONFIG_SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
}

void _ui_drawMainTop(ui_state_t * ui_state)
{

#ifdef CONFIG_RTC
    // Print clock on top bar
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_darkRed, "%02d:%02d:%02d", local_time.hour,
              local_time.minute, local_time.second);

    // if(history_size(history_list)==0) {
    //     history_add(history_list, "M0VVA", "NONE", local_time);
    //     history_add(history_list, "G4XIX", "NONE", local_time);
    //     history_add(history_list, "M7TFT", "NONE", local_time);
    //     history_add(history_list, "G0WCZ", "NONE", local_time);
    //     history_add(history_list, "2E0MXA", "NONE", local_time);
    //     history_add(history_list, "MM7RBK", "NONE", local_time);
    // }

#endif

    // If the radio has no built-in battery, print input voltage
#ifdef CONFIG_BAT_NONE
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_RIGHT,
              color_white,"%.1fV", last_state.v_bat);
#else
    if(last_state.settings.display_battery) {
            // Otherwise print battery icon on top bar, use 4 px padding
            uint16_t bat_width = CONFIG_SCREEN_WIDTH / 9;
            uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
            point_t bat_pos = {CONFIG_SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                            layout.status_v_pad};
            gfx_drawBattery(bat_pos, bat_width, bat_height, last_state.charge);
    } else {
            point_t bat_pos = {layout.top_pos.x, layout.top_pos.y - 2};
            gfx_print(bat_pos , FONT_SIZE_6PT, TEXT_ALIGN_RIGHT,
                  color_white,"%d%%", last_state.charge);
    }
#endif

    if (ui_state->input_locked == true)
      gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_LEFT,
                     color_white, SYMBOL_LOCK);

    point_t list_pos = {layout.top_pos.x + 24, layout.top_pos.y};
    if (last_state.settings.history_enabled)
    {
        gfx_print(list_pos , layout.line1_font, TEXT_ALIGN_LEFT,
            is_new_history() ? yellow_fab413 : color_black, "H");
//            yellow_fab413, "H%d", history_size(history_list));
        rtx_setHistory(is_new_history());
    }
}

void _ui_drawBankChannel()
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%01d-%03d: %.12s",
              b, last_state.channel_index + 1, last_state.channel.name);
}

void _ui_drawModeInfo(ui_state_t* ui_state)
{
    char bw_str[8] = { 0 };
    char encdec_str[9] = { 0 };

    switch(last_state.channel.mode)
    {
        case OPMODE_FM:

            // Get Bandwidth string
            if(last_state.channel.bandwidth == BW_12_5)
                sniprintf(bw_str, 8, "NFM");
            else if(last_state.channel.bandwidth == BW_25)
                sniprintf(bw_str, 8, "FM");

            // Get encdec string
            bool tone_tx_enable = last_state.channel.fm.txToneEn;
            bool tone_rx_enable = last_state.channel.fm.rxToneEn;

            if (tone_tx_enable && tone_rx_enable)
                sniprintf(encdec_str, 9, "ED");
            else if (tone_tx_enable && !tone_rx_enable)
                sniprintf(encdec_str, 9, " E");
            else if (!tone_tx_enable && tone_rx_enable)
                sniprintf(encdec_str, 9, " D");
            else
                sniprintf(encdec_str, 9, "  ");

            // Print Bandwidth, Tone and encdec info
            if (tone_tx_enable || tone_rx_enable)
            {
                uint16_t tone = ctcss_tone[last_state.channel.fm.txTone];
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s %d.%d %s", bw_str, (tone / 10),
                          (tone % 10), encdec_str);
            }
            else
            {
                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", bw_str );
            }
            break;

        case OPMODE_DMR:
            // Print talkgroup
            gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                    color_white, "DMR TG%s", "");
            break;

        #ifdef CONFIG_M17
        case OPMODE_M17:
        {
            // if(history_size(history_list) == 0) {
            //     // Add some sample data
                // history_add(history_list, "M0VVA", "MOD_A", utcToLocalTime(last_state.time, last_state.settings.utc_timezone));
            //     history_add(history_list, "G4XIX", "MOD_B", utcToLocalTime(last_state.time, last_state.settings.utc_timezone));
            //     // Cleanup above
            // }

            // Print M17 Destination ID on line 3 of 3
            rtxStatus_t rtxStatus = rtx_getCurrentStatus();

            if(rtxStatus.lsfOk)
            {
                // Destination address
                gfx_drawSymbol(layout.line2_pos, layout.line2_symbol_size, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_RECEIVED);

                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          yellow_fab413, "%s", rtxStatus.M17_dst);

                // Source address
                gfx_drawSymbol(layout.line1_pos, layout.line1_symbol_size, TEXT_ALIGN_LEFT,
                               color_white, SYMBOL_CALL_MADE);

                gfx_print(layout.line1_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "%s", rtxStatus.M17_src);

                // RF link (if present)
                if(rtxStatus.M17_link[0] != '\0')
                {
                    gfx_drawSymbol(layout.line4_pos, layout.line3_symbol_size, TEXT_ALIGN_LEFT,
                                   color_white, SYMBOL_ACCESS_POINT);

                    gfx_print(layout.line4_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_blue, "%s", rtxStatus.M17_link);
                }

                // Reflector (if present)
                if(rtxStatus.M17_refl[0] != '\0')
                {
                    gfx_drawSymbol(layout.line3_pos, layout.line4_symbol_size, TEXT_ALIGN_LEFT,
                                   color_white, SYMBOL_NETWORK);

                    gfx_print(layout.line3_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                              color_white, "%s", rtxStatus.M17_refl);
                }
  		
	       // NEW VERSION	
                if (    rtxStatus.M17_dst != NULL && 
			rtxStatus.M17_src != NULL && 
			rtxStatus.M17_refl != NULL &&
                	last_state.settings.callsign != NULL ) {
                
                    datetime_t local_time = utcToLocalTime(last_state.time, last_state.settings.utc_timezone);
                
                    bool isInfo = (strncmp(rtxStatus.M17_dst, "INFO", 4) == 0);
                    bool isEcho = (strncmp(rtxStatus.M17_dst, "ECHO", 4) == 0);
                    bool isSelf = (strncmp(rtxStatus.M17_src, last_state.settings.callsign, 8) == 0);
                
                    if (!isInfo && !isEcho && !isSelf) {
                        history_add(rtxStatus.M17_src, local_time);
	            }
                }                  
    
            }
            else
            {
                const char *dst = NULL;
                if(ui_state->edit_mode)
                {
                    dst = ui_state->new_callsign;
                }
                else
                {
                    if(strnlen(rtxStatus.destination_address, 10) == 0)
                        dst = currentLanguage->broadcast;
                    else
                        dst = rtxStatus.destination_address;
                }

                gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                          color_white, "M17 #%s", dst);
            }
            break;
        }
        #endif
    }
}

void _ui_drawFrequency()
{
    freq_t freq = platform_getPttStatus() ? last_state.channel.tx_frequency
                                          : last_state.channel.rx_frequency;

    // Print big numbers frequency
    char freq_str[16] = {0};
    sniprintf(freq_str, sizeof(freq_str), "%lu.%06lu", (freq / 1000000lu), (freq % 1000000lu));
    stripTrailingZeroes(freq_str);

    gfx_print(layout.line3_large_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
              color_white, "%s", freq_str);
}

void _ui_drawVFOMiddleInput(ui_state_t* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Rx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___.____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_rx_freq_buf);
        }
        gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency/1000000,
                  (unsigned long)(last_state.channel.tx_frequency%1000000)/100);
    }
    else if(ui_state->input_set == SET_TX)
    {
        gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Rx:%03lu.%04lu",
                  (unsigned long)ui_state->new_rx_frequency/1000000,
                  (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Tx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___.____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_tx_freq_buf);
        }
    }
}

void _ui_drawMainBottom()
{
    // Squelch bar
    rssi_t   rssi = last_state.rssi;
    uint8_t  squelch = last_state.settings.sqlLevel;
    uint8_t  volume = last_state.volume;
    uint16_t meter_width = CONFIG_SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad,
                          CONFIG_SCREEN_HEIGHT - meter_height - layout.bottom_pad};
    uint8_t mic_level = platform_getMicLevel();
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
            gfx_drawSmeter(meter_pos,
                           meter_width,
                           meter_height,
                           rssi,
                           squelch,
                           volume,
                           true,
                           yellow_fab413);
            break;
        case OPMODE_DMR:
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level,
                                volume,
                                true);
            break;
        #ifdef CONFIG_M17
	    case OPMODE_M17:
            if (last_state.rtxStatus==RTX_TX) {
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level,
                                volume,
                                true);
            } else if (last_state.settings.showSMeter) {
            gfx_drawSmeter(meter_pos,
                           meter_width,
                           meter_height,
                           rssi,
                           squelch,
                           volume,
                           true,
                           yellow_fab413);
            } else {
	    gfx_drawVolume(meter_pos, meter_width, meter_height, volume);
	    }
            break;
        #endif
    }
}

void _ui_drawMainVFO(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawModeInfo(ui_state);

    #ifdef CONFIG_M17
    // Show VFO frequency if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if((status.opMode != OPMODE_M17) || (status.lsfOk == false))
    #endif
        _ui_drawFrequency();

    _ui_drawMainBottom();
}

void _ui_drawMainVFOInput(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawVFOMiddleInput(ui_state);
    _ui_drawMainBottom();
}

void _ui_drawMainMEM(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawModeInfo(ui_state);

    #ifdef CONFIG_M17
    // Show channel data if the OpMode is not M17 or there is no valid LSF data
    rtxStatus_t status = rtx_getCurrentStatus();
    if((status.opMode != OPMODE_M17) || (status.lsfOk == false))
    #endif
    {
        _ui_drawBankChannel();
        _ui_drawFrequency();
    }

    _ui_drawMainBottom();
}
