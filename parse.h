/**
 * @file parse.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief The parser: turns lines of BASIC into program lines and runs immediate ones.
 * @details The grammar follows Carol Shaw's BNF for Atari BASIC (4/18/80).
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef PARSE_H
#define PARSE_H

/**
 * @brief Reads and handles input until BYE or the end of input.
 * @details A line starting with a line number is stored in the program (or,
 * with nothing after the number, deletes that line); any other line is run
 * at once. A line with a syntax error is reported and ignored.
 */
extern void parseProgram(void);

#endif // PARSE_H
