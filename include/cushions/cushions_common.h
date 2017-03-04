/**
 * @file cushions_common.h
 * @brief Header for use in other public API headers, should not be used
 * directly.
 */
#ifndef CUSHIONS_CUSHIONS_COMMON_H_
#define CUSHIONS_CUSHIONS_COMMON_H_

/**
 * @def CUHSIONS_PRINTF
 * @brief Specifies that a function takes printf style arguments which should be
 * type-checked against a format string.
 * @param a Index of the format string, in the parameters list.
 * @param b Index of the first argument to be checked against the format.
 */
#define CUHSIONS_PRINTF(a, b) __attribute__((format(printf, (a), (b))))

/**
 * @def CUSHIONS_UNUSED
 * @brief Marks a variable as possibly unused, avoid compiler's warnings.
 */
#define CUSHIONS_UNUSED __attribute__((unused))

/**
 * @def CUSHIONS_API
 * @brief Macro used to make the public API symbols visible, to be used in
 * conjunction with the -fvisibility=hidden flag passed at compilation time.
 */
#define CUSHIONS_API __attribute__((visibility("default")))

#endif /* INCLUDE_CUSHIONS_CUSHIONS_COMMON_H_ */
