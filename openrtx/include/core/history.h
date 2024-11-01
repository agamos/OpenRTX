/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef HISTORY_H
#define HISTORY_H

#include <datatypes.h>
#include <settings.h>
#include <pthread.h>
#include <stdbool.h>
#include <gps.h>

#define HISTORY_MAX 10

typedef struct history 
{
    char callsign[10];
    char module[10];
    datetime_t time;
    struct history * next;
    struct history * prev;
}
history_t;

typedef struct history_list
{
    history_t *head;
    history_t *tail;
    uint8_t length;
}
history_list_t;

// extern pthread_mutex_t history_mutex;
extern history_list_t *history_list;

/**
 * Initialise radio state mutex and radio state variable, reading the
 * informations from device drivers.
 */
history_list_t *history_init();
void history_add(history_list_t *list, const char* callsign, const char* module, datetime_t state_time);
int read_history(history_list_t *list, history_t *history, uint8_t pos);
history_t* find_callsign(history_list_t *list, const char* callsign);
uint8_t history_size(history_list_t *list);
void format_history_value(char *buf, int max_len, history_t history);
bool is_new_history();
void reset_new_history();

#endif /* HISTORY_H */
