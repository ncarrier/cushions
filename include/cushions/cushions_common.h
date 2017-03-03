/**
 * @file cushions_common.h
 * @brief Header for use in other public API headers, should not be used
 * directly.
 */
#ifndef CUSHIONS_CUSHIONS_COMMON_H_
#define CUSHIONS_CUSHIONS_COMMON_H_

/**
 * @def CUSHIONS_API
 * @brief Macro used to make the public API symbols visible, to be used in
 * conjunction with the -fvisibility=hidden flag passed at compilation time.
 */
#define CUSHIONS_API __attribute__((visibility("default")))

#endif /* INCLUDE_CUSHIONS_CUSHIONS_COMMON_H_ */
