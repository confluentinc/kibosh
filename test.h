/**
 * Copyright 2017 Confluent Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#ifndef KIBOSH_TEST_H
#define KIBOSH_TEST_H

#include "log.h" // for safe_strerror

#include <stdio.h> // for fprintf

/**
 * Abort unless t is true
 *
 * @param t		condition to check
 */
void die_unless(int t);

/**
 * Abort if t is true
 *
 * @param t		condition to check
 */
void die_if(int t);

/**
 * Create a zero-size file at ${file_name}
 *
 * @param fname		The file name
 *
 * @return		    0 on success; error code otherwise
 */
int do_touch1(const char *fname);

/**
 * Create a zero-size file at ${tempdir}/${file_name}
 *
 * @param dir		The dir
 * @param fname		The file name
 *
 * @return		    0 on success; error code otherwise
 */
int do_touch2(const char *dir, const char *fname);

/**
 * Expect two strings to be equal.
 *
 * @param a         The first string.
 * @param b         The second string.
 *
 * @return          0 on success; error code otherwise.
 */
int expect_str_equal(const char *a, const char *b);

#define EXPECT_INT_ZERO(x) \
    do { \
        int __my_ret__ = x; \
        if (__my_ret__) { \
            fprintf(stderr, "expected 0, got %d on line %d: %s\n",\
                __my_ret__, __LINE__, #x); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_NONZERO(x) \
    do { \
        int __my_ret__ = x; \
        if (__my_ret__ == 0) { \
            fprintf(stderr, "expected non-zero, got %d on line %d: %s\n",\
                __my_ret__, __LINE__, #x); \
            return -1; \
        } \
    } while (0);

#define EXPECT_NULL(x) \
    do { \
        void *__my_ret__ = x; \
        if (__my_ret__) { \
            fprintf(stderr, "expected NULL, got %p on line %d: %s\n",\
                __my_ret__, __LINE__, #x); \
            return -1; \
        } \
    } while (0);

#define EXPECT_NONNULL(x) \
    do { \
        void *__my_ret__ = x; \
        if (!__my_ret__) { \
            fprintf(stderr, "expected non-NULL, got NULL on line %d: %s\n",\
                __LINE__, #x); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_NONNEGATIVE(x) \
    do { \
        int __my_ret__ = x; \
        if (__my_ret__ < 0) { \
            fprintf(stderr, "expected non-negative, got %d on line %d: %s\n",\
                __my_ret__, __LINE__, #x); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_EQ(x, y) \
    do { \
        int __my_x__ = x; \
        int __my_y__ = y; \
        if (__my_x__ != __my_y__) { \
            fprintf(stderr, "expected %d, got %d on line %d: %s != %s\n",\
                    __my_x__, __my_y__, __LINE__, #x, #y); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_NE(x, y) \
    do { \
        int __my_x__ = x; \
        int __my_y__ = y; \
        if (__my_x__ == __my_y__) { \
            fprintf(stderr, "expected %d != %d on line %d: %s == %s\n",\
                    __my_x__, __my_y__, __LINE__, #x, #y); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_LT(x, y) \
    do { \
        int __my_x__ = x; \
        int __my_y__ = y; \
        if (__my_x__ >= __my_y__) { \
            fprintf(stderr, "expected %d < %d on line %d: %s >= %s\n",\
                __my_x__, __my_y__, __LINE__, #x, #y); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_GE(x, y) \
    do { \
        int __my_x__ = x; \
        int __my_y__ = y; \
        if (__my_x__ < __my_y__) { \
            fprintf(stderr, "expected %d >= %d on line %d: %s < %s\n",\
                __my_x__, __my_y__, __LINE__, #x, #y); \
            return -1; \
        } \
    } while (0);

#define EXPECT_INT_GT(x, y) \
    do { \
        int __my_x__ = x; \
        int __my_y__ = y; \
        if (__my_x__ <= __my_y__) { \
            fprintf(stderr, "expected %d > %d on line %d: %s <= %s\n",\
                __my_x__, __my_y__, __LINE__, #x, #y); \
            return -1; \
        } \
    } while (0);

#define EXPECT_POSIX_SUCC(expr) \
    do { \
        if (expr) { \
            int err = errno; \
            fprintf(stderr, "error %d (%s) on line %d: %s\n", \
                err, safe_strerror(err), __LINE__, #expr); \
            return -1; \
        } \
    } while (0);


#define EXPECT_POSIX_FAIL(expr, eret) \
    do { \
        int err; \
        \
        if (expr == 0) { \
            fprintf(stderr, "unexpected success on line %d: %s\n",\
                __LINE__, #expr); \
            return -1; \
        } \
        err = errno; \
        if (err != eret) { \
            fprintf(stderr, "unexpected error %d (%s) on line %d: %s\n",\
                err, safe_strerror(err), __LINE__, #expr); \
            return -1; \
        } \
    } while (0);

#define EXPECT_STR_EQ(a, b)  \
    do { \
        if (expect_str_equal(a, b)) { \
            fprintf(stderr, "failed on line %d: string comparison failed.  " \
                    "strcmp(%s, %s) != 0\n", \
                __LINE__, #a, #b); \
            return -1; \
        } \
    } while (0);

#endif

// vim: ts=4:sw=4:tw=99:et
