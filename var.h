/**
 * @file var.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Variables and the DATA list.
 * @details Variables follow Atari BASIC (Reference Manual, sections 7 and 8):
 * - A numeric variable used without a subscript is a scalar. It needn't be
 *   declared, and reads as 0 until set.
 * - A numeric array must be DIMensioned first. DIM A(10) gives subscripts 0
 *   to 10; DIM M(6,3) gives 0-6 by 0-3.
 * - A string variable (name ending in '$') must be DIMensioned first. DIM
 *   A$(20) makes one string of up to 20 characters; substrings are handled by
 *   the caller.
 * - A name can't be DIMensioned twice (CLR first).
 *
 * Errors are reported with utilErrorN() and the Atari error number.
 * Subscripts are passed as 0-based indexes, with -1 meaning "no subscript".
 * @version 0.0.1
 * @date 2017-03-15
 * @copyright Copyright (c) 2026
 */

#ifndef VAR_H
#define VAR_H

/**
 * @brief Adds a DATA line's items to the end of the DATA list.
 * @details A line whose items are already in the list is skipped, so DATA
 * lines collected by RUN aren't added again when they are executed.
 * @param d The items, ending with a NULL pointer.
 * @param lineNum The DATA statement's line, for RESTORE.
 * @return 0, or 1 if memory runs out.
 */
extern int varAppendData(char **d, long int lineNum);

/**
 * @brief Forgets all variables and the DATA list, for RUN and NEW.
 */
extern void varClearAll(void);

/**
 * @brief Forgets all variables but keeps the DATA list, for CLR.
 */
extern void varClearVariables(void);

/**
 * @brief DIMensions an array or a string.
 * @details A numeric scalar of the same name is replaced by the array.
 * @param name The variable.
 * @param dim1 The largest first subscript, or for a string its maximum length.
 * @param dim2 The largest second subscript, or -1 if not given (it must be
 * -1 for a string).
 * @return 0, or 1 after reporting an error.
 */
extern int varDim(const char *name, long dim1, long dim2);

/**
 * @brief Returns a variable's value, or an array element's.
 * @details A value never set reads as "0", or "" for a string.
 * @param name The variable.
 * @param i The first subscript, or -1 for none.
 * @param j The second subscript, or -1 for none.
 * @return A newly allocated copy of the value, or NULL after reporting an error.
 */
extern char *varGetValue(const char *name, long i, long j);

/**
 * @brief Returns the DIM size of a string variable.
 * @param name The string variable.
 * @return Its maximum length, or -1 after reporting that it isn't dimensioned.
 */
extern long varStringMax(const char *name);

/**
 * @brief Initializes the module.
 * @details Arranges for every variable to be freed at exit.
 * @return 0.
 */
extern int varInit(void);

/**
 * @brief Reads the next DATA item.
 * @return A newly allocated copy of the item, or NULL when the data runs out.
 */
extern char *varReadData(void);

/**
 * @brief Sets where the next READ takes its data from.
 * @param lineNum The first DATA item on or after this line, or -1 for the
 * first item of all.
 * @return 0.
 */
extern int varRestoreData(long int lineNum);

/**
 * @brief Sets a variable, or an array element.
 * @details An unknown numeric scalar is created. A string is cut to its DIM
 * size.
 * @param name The variable.
 * @param value The new value. NULL is the same as "".
 * @param i The first subscript, or -1 for none.
 * @param j The second subscript, or -1 for none.
 * @return 0, or 1 after reporting an error.
 */
extern int varSetValue(const char *name, const char *value, long i, long j);

#endif // VAR_H
