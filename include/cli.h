/**
 * @file cli.h
 * @brief Command-line interface and argument parsing.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1: Operating System Interface and Command Execution.
 */

#ifndef CLI_H
#define CLI_H

#include "config.h"

/* Parse command-line arguments and populate config */
int cli_parse(int argc, char *argv[], stress_config_t *cfg);

/* Display application help banner and option explanations */
void cli_print_help(const char *prog_name);

/* Display version and platform information */
void cli_print_version(void);

/* Display dry-run simulation parameters and system analysis */
void cli_print_dry_run(const stress_config_t *cfg);

#endif /* CLI_H */
