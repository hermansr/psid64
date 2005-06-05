#ifndef ALREADY_INCLUDED_LOG_H
#define ALREADY_INCLUDED_LOG_H
/*
 * Copyright (c) 2002, 2003 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

enum log_level {
    LOG_MIN = -99,
    LOG_FATAL = -40,
    LOG_ERROR = -30,
    LOG_WARNING = -20,
    LOG_BRIEF = -10,
    LOG_NORMAL = 0,
    LOG_VERBOSE = 10,
    LOG_TRACE = 20,
    LOG_DEBUG = 30,
    LOG_DUMP = 40,
    LOG_MAX = 99
};

/* disable all logging */
#define LOG(L, M)

#endif
