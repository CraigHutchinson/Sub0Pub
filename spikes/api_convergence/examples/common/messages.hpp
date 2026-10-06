#pragma once
/** Messages of the weather-station examples.
 *
 * Every candidate API in this directory carries these two types, so the examples differ only in how their
 * participants are written and composed. See any candidate's main.cpp for the story.
 */

struct Reading
{
    int celsius;
};

struct Alarm
{
    int celsius;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm
