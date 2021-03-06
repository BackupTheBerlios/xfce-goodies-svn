
#ifndef UTILS_H
#define UTILS_H

/**
 * Formats the number according the current locale with thousands separator. E.g.
 * 1234.5678 is formated in a German locale with digits=2 to 1.234,57. If the size
 * is too small, <code>NULL</code> is returned and the string contains garbage.
 * @param   string      a character array in which the result is stored
 * @param   stringsize  the size of the character array
 * @param   number      the number that should be formated
 * @param   digits      the number of digits after the decimal point
 * @return  the string to allow concatening buffers or <code>null</code>
 */
char* format_with_thousandssep( char* string, int stringsize, double number, int digits );

#endif /* UTILS_H */
