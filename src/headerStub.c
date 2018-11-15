// Header
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>

int INVALID_RAND;

int msqrt(int x)
{
    // Base cases
    if (x == 0 || x == 1)
       return x;

    // Do Binary Search for floor(sqrt(x))
    int start = 1, end = x, ans;
    while (start <= end)
    {
        int mid = (start + end) / 2;

        // If x is a perfect square
        if (mid*mid == x)
            return mid;

        // Since we need floor, we update answer when mid*mid is 
        // smaller than x, and move closer to sqrt(x)
        if (mid*mid < x)
        {
            start = mid + 1;
            ans = mid;
        }
        else // If mid*mid is greater than x
            end = mid - 1;
    }
    return ans;
}

int rand_a_b(int a, int b){
    if (b == INT_MAX) {
	return a + rand();
    }
    if (a == INT_MIN) {
	return (rand() - INT_MAX) % b;
    }
    if ((b-a)+1 <= 0) {
	INVALID_RAND = 1;
	fprintf(stderr, "range void [%d, %d].\n", a, b);
	return 0;
    }
    return rand()%(b-a+1) + a;
}

int max(int numArgs, ...)
{
    va_list args;
    va_start(args, numArgs);

    if (numArgs <= 0) {
      va_end(args);
      printf("[headerStub] Invalid call of max(int numArgs, ...): not enough arguments.\n");
      return 0;
    }

    int m = va_arg(args, int);

    for(unsigned int i = 1; i < numArgs; ++i)
    {
        int a = va_arg(args, int);
        if (a > m)
            m = a;
    }

    va_end(args);

    return m;
}

int min(int numArgs, ...)
{
    va_list args;
    va_start(args, numArgs);

    if (numArgs <= 0) {
      va_end(args);
      printf("[headerStub] Invalid call of min(int numArgs, ...): not enough arguments.\n");
      return 0;
    }

    int m = va_arg(args, int);

    for(unsigned int i = 1; i < numArgs; ++i)
    {
        int a = va_arg(args, int);
        if (a < m)
            m = a;
    }

    va_end(args);

    return m;
}

int overflow(int *term1, int *term2) {
  int tmp = *term2 * *term1;
  // Overflow ?
  if ((*term1 != 0 && tmp / *term1 != *term2) ||
      (*term2 != 0 && tmp / *term2 != *term1)) {
    return 1;
  }
  return 0;
}

void overflow_check(int *term1, int *term2, int only_left) {
  int test = 1;
  while (test) {
    test = 0;
    test = overflow(term1, term2);
    if (only_left) {
      *term1 = *term1 / 2;
    } else {
      if (rand() % 2) 
        *term1 = *term1 / 2;
      else
        *term2 = *term2 / 2;
    }
  }
}

// low <= term1 * term2
/* Either
 *   term1 != 0 or set to u1
 *   term2 = max(term2, low / term1)
 * Or
 *   term2 != 0 or set to u2
 *   term1 = max(term1, low / term2)
 */
void fix_product_is_greater(int low, int *term1, int u1, int *term2, int u2)
{
    // Ignore if the constraint is met
    if (*term2 * *term1 >= low) {
        //printf("%d <= %d x %d\n", low, *term1, *term2);
        return;
    }
    if (rand() % 2) {
        if (*term1 == 0)
            *term1 = u1;
        *term2 = max(2, *term2, low / *term1);
    } else {
        if (*term2 == 0)
            *term2 = u2;
        *term1 = max(2, *term1, low / *term2);
    }
    // TODO : The divider must be bounded too
}

// term1 * term2 <= up
/* Either
 *   term1 != 0 or set to u1
 *   term2 = min(term2, up / term1)
 * Or
 *   term2 != 0 or set to u2
 *   term1 = min(term1, up / term2)
 */
void fix_product_is_lesser(int up, int *term1, int u1, int *term2, int u2)
{
    // Ignore if the constraint is met
    if (*term2 * *term1 <= up) {
        //printf("%d x %d <= %d\n", *term1, *term2, up);
        return;
    }
    if (rand() % 2) {
        if (*term1 == 0)
            *term1 = u1;
        *term2 = min(2, *term2, up / *term1);
    } else {
        if (*term2 == 0)
            *term2 = u2;
        *term1 = min(2, *term1, up / *term2);
    }
}

void fix_product_terms(int *left_low, int *left_up, int *right_low, int *right_up, int product_low, int product_up)
{
    //printf("[%d, %d] x [%d, %d] in [%d, %d]\n", *left_low, *left_up, *right_low, *right_up, product_low, product_up);
    // We cannot devide a positive lower bound or a negative upper bound
    if (overflow(left_low, right_low)) {
      if (*left_low <= 0 && *right_low <= 0) {
        overflow_check(left_low, right_low, 0);
      } else if (*left_low > 0 && *right_low <= 0) {
        overflow_check(right_low, left_low, 1);
      } else if (*right_low > 0 && *left_low <= 0) {
        overflow_check(left_low, right_low, 1);
      } else {
        printf("Lower bounds (ll, rl) provoke overflow: %d x %d = %d\n", *left_low, *right_low, *left_low * *right_low);
        INVALID_RAND = 1;
      }
    }
    if (overflow(left_low, right_up)) {
      if (*left_low <= 0 && *right_up >= 0) {
        overflow_check(left_low, right_up, 0);
      } else if (*left_low > 0 && *right_up >= 0) {
        overflow_check(right_up, left_low, 1);
      } else if (*right_up < 0 && *left_low <= 0) {
        overflow_check(left_low, right_up, 1);
      } else {
        printf("Bounds (ll, ru) provoke overflow: %d x %d = %d\n", *left_low, *right_up, *left_low * *right_up);
        INVALID_RAND = 1;
      }
    }
    if (overflow(right_low, left_up)) {
      if (*right_low <= 0 && *left_up >= 0) {
        overflow_check(right_low, left_low, 0);
      } else if (*right_low > 0 && *left_up >= 0) {
        overflow_check(left_up, right_low, 1);
      } else if (*left_up < 0 && *right_low <= 0) {
        overflow_check(right_low, left_up, 1);
      } else {
        printf("Bounds (rl, lu) provoke overflow: %d x %d = %d\n", *right_low, *left_up, *right_low * *left_up);
        INVALID_RAND = 1;
      }
    }
    if (overflow(left_up, right_up)) {
      if (*left_up >= 0 && *right_up >= 0) {
        overflow_check(left_up, right_up, 0);
      } else if (*left_up < 0 && *right_up >= 0) {
        overflow_check(right_up, left_up, 1);
      } else if (*right_up < 0 && *left_up >= 0) {
        overflow_check(left_up, right_up, 1);
      } else {
        printf("Lower bounds (lu, ru) provoke overflow: %d x %d = %d\n", *left_up, *right_up, *left_up * *right_up);
        INVALID_RAND = 1;
      }
    }

    // Identify patterns
    // 1. Both var are positives
    if (*left_low >= 0 && *right_low >= 0) {
        // Low bound
        fix_product_is_greater(product_low, left_low, 1, right_low, 1);
        // Upper bound
        fix_product_is_lesser(product_up, left_up, -1, right_up, -1);
    }
    // 2. One is positive and the other can be both signs
    else if (*left_low <= 0 && *left_up >= 0 && *right_low >= 0) {
        // Low bound
        if (*right_up * *left_low < product_low) {
            if (*right_up == 0)
                *right_up = -1;
            *left_low = max(2, *left_low, product_low / *right_up);
        }
        // Upper bound
        fix_product_is_lesser(product_up, left_up, -1, right_up, -1);
    }
    else if (*right_low <= 0 && *right_up >= 0 && *left_low >= 0) {
        // Low bound
        if (*right_low * *left_up < product_low) {
            if (*left_up == 0)
                *left_up = -1;
            *right_low = max(2, *right_low, product_low / *left_up);
        }
        // Upper bound
        fix_product_is_lesser(product_up, right_up, -1, left_up, -1);
    }
    // 3. They can both be both signs
    else if (*left_low <= 0 && *left_up >= 0
             && *right_low <= 0 && *right_up >= 0) {
        // Low bound
        if (*right_up * *left_low < product_low) {
            if (*right_up == 0)
                *right_up = -1;
            *left_low = max(2, *left_low, product_low / *right_up);
        }
        if (*right_low * *left_up < product_low) {
            if (*left_up == 0)
                *left_up = -1;
            *right_low = max(2, *right_low, product_low / *left_up);
        }
        // Upper bound
        fix_product_is_greater(product_up, left_low, 1, right_low, 1);
        fix_product_is_lesser(product_up, left_up, -1, right_up, -1);
    }
    // 4. They are opposed signs
    else if (*right_low >= 0 && *left_up <= 0) {
        // Low bound
        if (*right_up * *left_low < product_low) {
            if (*right_up == 0)
                *right_up = -1;
            *left_low = max(2, *left_low, product_low / *right_up);
        }
        // Upper bound
        if (*right_low * *left_up > product_up) {
            if (*right_low == 0)
                *right_low = 1;
            *left_up = min(2, *left_up, product_up / *right_low);
        }
    }
    else if (*left_low >= 0 && *right_up <= 0) {
        // Low bound
        if (*right_low * *left_up < product_low) {
            if (*left_up == 0)
                *left_up = -1;
            *right_low = max(2, *right_low, product_low / *left_up);
        }
        // Upper bound
        if (*right_up * *left_low > product_up) {
            if (*left_low == 0)
                *left_low = 1;
            *right_up = min(2, *right_up, product_up / *left_low);
        }
    }
    // 5. Both are negatives
    else if (*left_up <= 0 && *right_up <= 0) {
        // Low bound
        fix_product_is_lesser(product_low, left_up, -1, right_up, -1);
        // Upper bound
        fix_product_is_greater(product_up, left_low, 1, right_low, 1);
    }
    else {
        fprintf(stderr, "Unexpected case for fix_product_terms\n");
        INVALID_RAND = 1;
    }
}

