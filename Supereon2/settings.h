#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile int   aimbot;          // Toggle
    volatile int   teamcheck;       // TeamCheck
    volatile int   duels;           // Duels
    volatile int   jumpbot;         // JumpbotEnabled
    volatile int   fov_check;       // FOVEnabled
    volatile float distance;        // JumpbotMaxDistance (aim distance)
    volatile float reach_margin;    // ReachMargin (0.0 = normal)
    volatile float wiggle_speed;    // WiggleSpeed
    volatile int   delay_ms;        // JumpbotDelay (aim delay)
    volatile float jump_dist;       // JumpbotMaxDistance (jump distance)
    volatile int   jump_delay_ms;   // JumpbotDelay (jump delay)
    volatile int   running;         // Set 0 to exit
} Settings;

extern Settings g_cfg;

#endif