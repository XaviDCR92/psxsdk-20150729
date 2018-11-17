#ifndef _ASSERT_H
#define _ASSERT_H

#define STATIC_ASSERT(expr) {enum{e = 1 / !!(expr)};}

#endif /* _ASSERT_H */
