/******************************************************************************
 * @file      utils.h
 * @brief     Utility macros (MIN, MAX, IN_RANGE, etc)
 * @version   1.2
 * @date      Aug 30, 2020
 * @copyright
 *****************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#define GET_VERSION(major, minor)		__GET_VERSION(major, minor)
#define __GET_VERSION(major, minor)		#major "." #minor


#define __TO_STR(x)						#x
#define TO_STR(x)						__TO_STR(x)

/*	check range, inclusive (including start and end points)	*/
#define IN_RANGE_IN(v, s, e)		(((s) <= (v)) && ((v) <= (e)))

/*	check range, exclusive (not including start and end points)	*/
#define IN_RANGE_EX(v, s, e)		(((s) < (v)) && ((v) < (e)))

/*	check range (including start and end points)	*/
#define IN_RANGE(v, s, e)			IN_RANGE_IN(v, s, e)

/*	check c is a numeric character [0-9]	*/
#define IS_NUM(c)					IN_RANGE(c, '0', '9')

/*	check c is a lower case alphabetical character [a-z]	*/
#define IS_LOWER(c)					IN_RANGE(c, 'a', 'z')

/*	check c is an upper case alphabetical character [A-Z]	*/
#define IS_UPPER(c)					IN_RANGE(c, 'A', 'Z')

/*	check c is an alphabetical character [a-zA-Z]	*/
#define IS_ALPHA(c)					(IS_UPPER(c) || IS_LOWER(c))

/*	check c is an alphanumeric character [a-zA-Z0-9]	*/
#define IS_ALPHANUM(c)				(IS_ALPHA(c) || IS_NUM(c))

/*	alphabets upper/lower case ascii offset	*/
#define ALPHA_LOWER_UPPER_DIFF		('a' - 'A')

/*	get upper case from lower case	*/
#define TO_UPPER(c)					((IS_LOWER(c)) ? ((c) - ALPHA_LOWER_UPPER_DIFF) : (c))

/*	get lower case from upper case	*/
#define TO_LOWER(c)					((IS_UPPER(c)) ? ((c) + ALPHA_LOWER_UPPER_DIFF) : (c))

/*	get integer digit from character digit	*/
#define CHAR_TO_INT(c)				((IS_NUM(c)) ? ((c) - 0) : (c))

/*	get character digit from integer digit	*/
#define INT_TO_CHAR(i)				((IN_RANGE(i, 0, 9)) ? ((i) + '0') : i)

/*	minimum of 2 values	*/
#define MIN(a, b)					(((a) < (b))? (a) : (b))

/*	maximum of 2 values	*/
#define MAX(a, b)					(((a) > (b))? (a) : (b))

/*	check nullptr	*/
#define IS_NULLPTR(ptr)				((ptr) == NULL)

/*	check zero value	*/
#define IS_ZERO(v)					((v) == 0)

#endif /* __UTILS_H__ */
