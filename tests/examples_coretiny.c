/**
 * ==============================================================================
 * CoreTiny Language Examples
 * ==============================================================================
 *
 * This file shows example programs in CoreTiny syntax alongside their C
 * equivalents to illustrate what the language looks like.
 * ==============================================================================
 */

/*
 * Example 1: Factorial
 * ====================
 *
 * CoreTiny:
 * ---------
 * func factorial(n: int) : int {
 *     var result = 1;
 *     while (n > 1) {
 *         result = result * n;
 *         n = n - 1;
 *     }
 *     return result;
 * }
 *
 * func main() : int {
 *     var x = 5;
 *     var fact = factorial(x);
 *     print(fact);
 *     return 0;
 * }
 *
 * C Equivalent:
 * -------------
 * int factorial(int n) {
 *     int result = 1;
 *     while (n > 1) {
 *         result = result * n;
 *         n = n - 1;
 *     }
 *     return result;
 * }
 *
 * int main(void) {
 *     int x = 5;
 *     int fact = factorial(x);
 *     printf("%d\n", fact);
 *     return 0;
 * }
 */

/*
 * Example 2: Greatest Common Divisor (GCD)
 * =========================================
 *
 * CoreTiny:
 * ---------
 * func gcd(a: int, b: int) : int {
 *     while (b != 0) {
 *         var temp = b;
 *         b = a % b;
 *         a = temp;
 *     }
 *     return a;
 * }
 *
 * func main() : int {
 *     var x = 48;
 *     var y = 18;
 *     var result = gcd(x, y);
 *     print(result);
 *     return 0;
 * }
 */

/*
 * Example 3: Is Prime
 * ===================
 *
 * CoreTiny:
 * ---------
 * func is_prime(n: int) : bool {
 *     if (n <= 1) {
 *         return false;
 *     }
 *
 *     var i = 2;
 *     while (i * i <= n) {
 *         if (n % i == 0) {
 *             return false;
 *         }
 *         i = i + 1;
 *     }
 *
 *     return true;
 * }
 *
 * func main() : int {
 *     var num = 17;
 *     var prime = is_prime(num);
 *     if (prime) {
 *         print(1);
 *     } else {
 *         print(0);
 *     }
 *     return 0;
 * }
 */

/*
 * Example 4: Fibonacci
 * ====================
 *
 * CoreTiny:
 * ---------
 * func fibonacci(n: int) : int {
 *     if (n <= 1) {
 *         return n;
 *     }
 *
 *     var a = 0;
 *     var b = 1;
 *     var i = 2;
 *
 *     while (i <= n) {
 *         var temp = a + b;
 *         a = b;
 *         b = temp;
 *         i = i + 1;
 *     }
 *
 *     return b;
 * }
 *
 * func main() : int {
 *     var n = 10;
 *     var result = fibonacci(n);
 *     print(result);
 *     return 0;
 * }
 */

/*
 * Example 5: Sum of Array Elements (using recursion)
 * ==================================================
 *
 * CoreTiny:
 * ---------
 * func sum_range(start: int, end: int) : int {
 *     if (start > end) {
 *         return 0;
 *     }
 *     return start + sum_range(start + 1, end);
 * }
 *
 * func main() : int {
 *     var total = sum_range(1, 100);
 *     print(total);
 *     return 0;
 * }
 */

/*
 * Example 6: Logical Operations
 * =============================
 *
 * CoreTiny:
 * ---------
 * func is_in_range(x: int, low: int, high: int) : bool {
 *     return (x >= low) && (x <= high);
 * }
 *
 * func is_even(x: int) : bool {
 *     return (x % 2) == 0;
 * }
 *
 * func main() : int {
 *     var num = 42;
 *
 *     if (is_in_range(num, 1, 100) && is_even(num)) {
 *         print(1);
 *     } else {
 *         print(0);
 *     }
 *
 *     return 0;
 * }
 */

/*
 * Example 7: Multiple Conditions
 * ==============================
 *
 * CoreTiny:
 * ---------
 * func classify_number(n: int) : int {
 *     if (n < 0) {
 *         return 0 - 1;
 *     } else {
 *         if (n == 0) {
 *             return 0;
 *         } else {
 *             return 1;
 *         }
 *     }
 * }
 *
 * func main() : int {
 *     var result1 = classify_number(0 - 5);
 *     var result2 = classify_number(0);
 *     var result3 = classify_number(5);
 *
 *     print(result1);
 *     print(result2);
 *     print(result3);
 *
 *     return 0;
 * }
 */

/*
 * Example 8: Power Function
 * =========================
 *
 * CoreTiny:
 * ---------
 * func power(base: int, exp: int) : int {
 *     var result = 1;
 *     var i = 0;
 *
 *     while (i < exp) {
 *         result = result * base;
 *         i = i + 1;
 *     }
 *
 *     return result;
 * }
 *
 * func main() : int {
 *     var result = power(2, 10);
 *     print(result);
 *     return 0;
 * }
 */

/*
 * Key Language Features Demonstrated:
 * ==================================
 *
 * 1. Function definitions with typed parameters
 * 2. Variable declarations with type inference from initializer
 * 3. Arithmetic operations: +, -, *, /, %
 * 4. Comparison operations: ==, !=, <, <=, >, >=
 * 5. Logical operations: &&, ||, !
 * 6. If/else statements
 * 7. While loops
 * 8. Function calls
 * 9. Return statements
 * 10. The built-in print() function
 *
 * Notable Restrictions:
 * ====================
 *
 * - No arrays or pointers
 * - No structs or user-defined types
 * - No for loops (use while)
 * - No break or continue
 * - No string literals (only int and bool)
 * - No global variables
 * - Function parameters are passed by value
 * - All variables must be initialized when declared
 */