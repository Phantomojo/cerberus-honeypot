/**
 * quorum_adapt.c - Quorum-based adaptation module
 * 
 * Detects coordinated attacks and triggers adaptive responses:
 * - Increase morphing frequency
 * - Add response delays
 * - Fake errors to confuse attackers
 * - Disconnect suspected coordinated attacks
 * 
 * WHY THIS MODULE EXISTS:
 * Think of this like a security guard who watches multiple cameras.
 * When they see the same suspicious person at different entrances,
 * they know it's a coordinated attack and sound the alarm.
 * This module IS that security guard for our honeypot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "quorum_adapt.h"
#include "utils.h"

// Signal file paths - these are how quorum tells morph "hey, do something!"
// Think of these like notes left on the fridge for your roommate
#define EMERGENCY_MORPH_SIGNAL "build/signals/emergency_morph.signal"
#define MORPH_FREQUENCY_FILE "build/signals/morph_frequency.conf"
#define ATTACKER_BLOCKLIST "build/signals/attacker_blocklist.txt"

// Known attack signatures (reserved for future pattern matching)
__attribute__((unused))
static const char* brute_force_signatures[] = {
    "Failed password for",
    "Invalid user",
    "Received disconnect",
    "Connection refused"
};

__attribute__((unused))
static const char* exploitation_signatures[] = {
    "buffer overflow",
    "injection",
    "shellcode",
    "ROP gadget",
    "privilege escalation"
};

__attribute__((unused))
static const char* reconnaissance_signatures[] = {
    "nmap",
    "masscan",
    "shodan",
    "censys",
    "port scan",
    "service discovery"
};

// Current threat metrics
static uint32_t current_morph_frequency_minutes = 360;  // Default: 6 hours
static uint32_t current_command_delay_ms = 0;           // Default: none
static bool emergency_mode = false;

/**
 * Create attack pattern
 */
attack_pattern_t* create_attack_pattern(const char* name) {
    attack_pattern_t* pattern = (attack_pattern_t*)malloc(sizeof(attack_pattern_t));
    if (!pattern) return NULL;

    memset(pattern, 0, sizeof(attack_pattern_t));
    pattern->pattern_name = name;
    pattern->severity = 5;
    pattern->first_seen = time(NULL);
    pattern->last_seen = time(NULL);

    return pattern;
}

/**
 * Create attacker profile
 */
attacker_profile_t* create_attacker_profile(const char* ip) {
    attacker_profile_t* profile = (attacker_profile_t*)malloc(sizeof(attacker_profile_t));
    if (!profile) return NULL;

    memset(profile, 0, sizeof(attacker_profile_t));
    if (ip) strncpy(profile->ip_address, ip, 31);
    profile->first_contact = time(NULL);
    profile->last_contact = time(NULL);

    return profile;
}

/**
 * Calculate threat score for an attacker (0.0-1.0)
 */
float calculate_threat_score(attacker_profile_t* attacker) {
    if (!attacker) return 0.0f;

    float score = 0.0f;

    // Frequency of attempts (higher = more threatening)
    if (attacker->total_attempts > 0) {
        score += (float)(attacker->total_attempts % 100) / 100.0f;
    }

    // Success rate (exploits are very bad)
    if (attacker->successful_exploits > 0) {
        score += (float)(attacker->successful_exploits * 10) / 100.0f;
    }

    // Is part of coordinated attack?
    if (attacker->is_coordinated) {
        score += 0.3f;
    }

    return score > 1.0f ? 1.0f : score;
}

/**
 * Detect if attackers are working together
 */
void detect_coordination(attacker_profile_t** attackers, int attacker_count) {
    if (!attackers || attacker_count < 2) return;

    // Look for similar patterns happening at similar times
    for (int i = 0; i < attacker_count; i++) {
        for (int j = i + 1; j < attacker_count; j++) {
            // Check if patterns overlap in time (within 5 minutes)
            time_t time_diff = abs((int)(attackers[i]->last_contact - attackers[j]->last_contact));
            
            // Similar pattern types and close timing = coordinated
            if (time_diff < 300 && attackers[i]->pattern_count > 0 && 
                attackers[j]->pattern_count > 0) {
                attackers[i]->is_coordinated = true;
                attackers[j]->is_coordinated = true;

                char msg[256];
                snprintf(msg, sizeof(msg), "Coordinated attack detected: %s and %s",
                         attackers[i]->ip_address, attackers[j]->ip_address);
                log_event_level(LOG_WARN, msg);
            }
        }
    }
}

/**
 * Check if there's a coordinated attack in progress
 */
bool is_coordinated_attack(attacker_profile_t** attackers, int attacker_count) {
    if (!attackers || attacker_count == 0) return false;

    int coordinated_count = 0;
    for (int i = 0; i < attacker_count; i++) {
        if (attackers[i]->is_coordinated) {
            coordinated_count++;
        }
    }

    // 2+ coordinated attackers = coordinated attack
    return coordinated_count >= 2;
}

/**
 * Assess overall threat level
 */
threat_assessment_t* assess_threat_level(attacker_profile_t** attackers, int attacker_count) {
    threat_assessment_t* threat = (threat_assessment_t*)malloc(sizeof(threat_assessment_t));
    if (!threat) return NULL;

    memset(threat, 0, sizeof(threat_assessment_t));
    threat->assessment_time = time(NULL);
    threat->total_unique_attackers = attacker_count;

    if (attacker_count == 0) {
        threat->overall_threat_level = 0.0f;
        return threat;
    }

    // Calculate average threat score
    float total_score = 0.0f;
    int coordinated = 0;

    for (int i = 0; i < attacker_count; i++) {
        total_score += calculate_threat_score(attackers[i]);
        if (attackers[i]->is_coordinated) coordinated++;
    }

    threat->overall_threat_level = total_score / attacker_count;
    threat->coordinated_attack_count = coordinated / 2;  // Pairs
    threat->should_trigger_rapid_morph = (threat->overall_threat_level > 0.6f) ||
                                         is_coordinated_attack(attackers, attacker_count);

    return threat;
}

/**
 * Get appropriate response action
 */
response_action_t get_appropriate_response(threat_assessment_t* threat) {
    if (!threat) return RESPONSE_NONE;

    if (threat->should_trigger_rapid_morph) {
        return RESPONSE_INCREASE_MORPHING_FREQUENCY;
    }

    if (threat->overall_threat_level > 0.7f) {
        return RESPONSE_FAKE_ERRORS;
    }

    if (threat->overall_threat_level > 0.5f) {
        return RESPONSE_ADD_DELAYS;
    }

    if (threat->coordinated_attack_count > 0) {
        return RESPONSE_DISCONNECT;
    }

    return RESPONSE_NONE;
}

/**
 * Update attack pattern with new log entry
 */
void update_attack_pattern(attack_pattern_t* pattern, const char* log_entry) {
    if (!pattern || !log_entry) return;

    pattern->occurrence_count++;
    pattern->last_seen = time(NULL);

    // Increase severity based on frequency
    if (pattern->occurrence_count > 100) pattern->severity = 9;
    else if (pattern->occurrence_count > 50) pattern->severity = 8;
    else if (pattern->occurrence_count > 20) pattern->severity = 7;
    else if (pattern->occurrence_count > 10) pattern->severity = 6;
}

/**
 * Trigger emergency morphing
 * 
 * WHY THIS FIX: Before, this function just logged a message and did nothing!
 * It's like having a fire alarm that makes noise but doesn't call the fire department.
 * 
 * Now it writes a signal file that tells the morph engine "MORPH NOW!"
 * The morph engine (or a cron job/systemd timer) checks for this file and acts.
 * 
 * Think of it like leaving an urgent note: the quorum engine writes the note,
 * and the morph engine reads it and takes action.
 */
void trigger_emergency_morph(void) {
    char msg[256];
    snprintf(msg, sizeof(msg), "EMERGENCY: Coordinated attack detected - triggering immediate morph!");
    log_event_level(LOG_WARN, msg);
    
    // Create the signals directory if it doesn't exist
    create_dir("build/signals");
    
    // Write emergency signal file with timestamp and reason
    // The morph engine or a monitoring script will see this and act
    char signal_content[512];
    time_t now = time(NULL);
    snprintf(signal_content, sizeof(signal_content),
        "# EMERGENCY MORPH SIGNAL\n"
        "# This file triggers an immediate device profile change\n"
        "timestamp=%ld\n"
        "reason=coordinated_attack_detected\n"
        "requested_by=quorum_engine\n"
        "priority=CRITICAL\n",
        now);
    
    if (write_file(EMERGENCY_MORPH_SIGNAL, signal_content) == 0) {
        log_event_level(LOG_INFO, "Emergency morph signal written successfully");
        
        // Also try to directly execute morph if binary exists
        // This is a backup - the signal file is the primary mechanism
        if (file_exists("build/morph")) {
            log_event_level(LOG_INFO, "Attempting direct morph execution...");
            int result = system("./build/morph profiles.conf 2>/dev/null &");
            if (result == 0) {
                log_event_level(LOG_INFO, "Direct morph execution initiated");
            }
        }
    } else {
        log_event_level(LOG_ERROR, "Failed to write emergency morph signal!");
    }
    
    emergency_mode = true;
}

/**
 * Increase morphing frequency
 * 
 * WHY THIS FIX: Before, this just changed a variable in memory that nobody read!
 * It's like turning up the thermostat in a house that's not connected to the heater.
 * 
 * Now it writes the new frequency to a config file that systemd timers or cron can read.
 * A helper script can then adjust the actual timer schedule.
 */
void increase_morphing_frequency(uint32_t new_frequency_minutes) {
    if (new_frequency_minutes < 1) new_frequency_minutes = 1;
    if (new_frequency_minutes > 1440) new_frequency_minutes = 1440; // Max 24 hours
    
    current_morph_frequency_minutes = new_frequency_minutes;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Morphing frequency changed to every %u minutes",
             new_frequency_minutes);
    log_event_level(LOG_WARN, msg);

    // Create signals directory
    create_dir("build/signals");
    
    // Write the new frequency to a config file
    // A systemd timer or cron job can read this and adjust its schedule
    char freq_content[256];
    snprintf(freq_content, sizeof(freq_content),
        "# Morph frequency configuration\n"
        "# Generated by quorum_adapt at %ld\n"
        "frequency_minutes=%u\n"
        "reason=%s\n",
        time(NULL),
        new_frequency_minutes,
        new_frequency_minutes < 60 ? "elevated_threat" : "normal_operation");
    
    write_file(MORPH_FREQUENCY_FILE, freq_content);
    
    // Log advice for the operator
    if (new_frequency_minutes <= 30) {
        log_event_level(LOG_WARN, "HIGH ALERT: Morphing every 30 min or less due to active threat");
    }
}

/**
 * Add command delays
 * 
 * WHY: Adding delays makes the honeypot feel more like a slow, cheap IoT device.
 * It also frustrates automated attack tools that expect fast responses.
 * We write this to the behavior config so Cowrie can read it.
 */
void add_command_delays(uint32_t delay_ms) {
    current_command_delay_ms = delay_ms;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Adding %u ms delays to all command responses", delay_ms);
    log_event_level(LOG_WARN, msg);
    
    // Update the behavior config file so Cowrie can apply these delays
    create_dir("build/cowrie-dynamic");
    
    char delay_config[256];
    snprintf(delay_config, sizeof(delay_config),
        "# Command delay configuration\n"
        "# Higher delays = slower responses = more realistic IoT device\n"
        "command_delay_ms=%u\n"
        "updated_at=%ld\n",
        delay_ms, time(NULL));
    
    // Append to or create the behavior config
    append_file("build/cowrie-dynamic/behavior.conf", delay_config);
}

/**
 * Simulate errors for specific attacker IP
 * 
 * WHY THIS FIX: Before, this did nothing! Now it adds the IP to a blocklist file.
 * Other components (Cowrie plugins, firewall scripts) can read this file and
 * return fake errors to these specific attackers.
 * 
 * It's like having a "naughty list" - people on the list get coal (errors) instead
 * of presents (real responses).
 */
void simulate_errors_for_attacker(const char* ip) {
    if (!ip) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "Adding attacker to error simulation list: %s", ip);
    log_event_level(LOG_WARN, msg);

    // Create signals directory
    create_dir("build/signals");
    
    // Add IP to blocklist file with timestamp
    // Format: IP|timestamp|reason
    char blocklist_entry[256];
    snprintf(blocklist_entry, sizeof(blocklist_entry),
        "%s|%ld|coordinated_attack\n", ip, time(NULL));
    
    append_file(ATTACKER_BLOCKLIST, blocklist_entry);
    
    // Log what we're doing to this attacker
    snprintf(msg, sizeof(msg), 
        "Attacker %s will now receive fake errors and slow responses", ip);
    log_event_level(LOG_INFO, msg);
}

/**
 * Log attack intelligence
 */
void log_attack_intelligence(attack_pattern_t* pattern, attacker_profile_t* attacker) {
    if (!pattern || !attacker) return;

    char intel_file[512];
    snprintf(intel_file, sizeof(intel_file), "build/attack-intel-%ld.log", time(NULL));

    FILE* f = fopen(intel_file, "a");
    if (!f) return;

    fprintf(f, "=== Attack Intelligence ===\n");
    fprintf(f, "Timestamp: %ld\n", time(NULL));
    fprintf(f, "Attacker IP: %s\n", attacker->ip_address);
    fprintf(f, "Pattern: %s\n", pattern->pattern_name);
    fprintf(f, "Severity: %u/10\n", pattern->severity);
    fprintf(f, "Occurrences: %u\n", pattern->occurrence_count);
    fprintf(f, "Attacker Attempts: %u\n", attacker->total_attempts);
    fprintf(f, "Successful Exploits: %u\n", attacker->successful_exploits);
    fprintf(f, "Coordinated: %s\n", attacker->is_coordinated ? "YES" : "NO");
    fprintf(f, "\n");

    fclose(f);
}

/**
 * Free attack pattern
 */
void free_attack_pattern(attack_pattern_t* pattern) {
    if (pattern) {
        free(pattern);
    }
}

/**
 * Free attacker profile
 */
void free_attacker_profile(attacker_profile_t* profile) {
    if (profile) {
        free(profile);
    }
}

/**
 * Export current threat metrics
 */
uint32_t get_current_morph_frequency(void) {
    return current_morph_frequency_minutes;
}

uint32_t get_current_command_delay(void) {
    return current_command_delay_ms;
}

bool is_in_emergency_mode(void) {
    return emergency_mode;
}

void reset_emergency_mode(void) {
    emergency_mode = false;
    current_morph_frequency_minutes = 360;  // Back to 6 hours
    current_command_delay_ms = 0;
    char msg[256];
    snprintf(msg, sizeof(msg), "Emergency mode disabled, returning to normal operation");
    log_event_level(LOG_INFO, msg);
}
