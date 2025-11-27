// Test: Various expression types
// Tests BinaryOperator, UnaryOperator, CallExpr, CastExpr, etc.

int test_expressions() {
    // Binary operators
    int a = 5 + 3;
    int b = a * 2;
    int c = b / a;
    int d = c % 2;
    
    // Comparison operators
    bool eq = (a == b);
    bool ne = (a != b);
    bool lt = (a < b);
    bool gt = (a > b);
    bool le = (a <= b);
    bool ge = (a >= b);
    
    // Logical operators
    bool and_result = eq && ne;
    bool or_result = lt || gt;
    bool not_result = !eq;
    
    // Bitwise operators
    int bit_and = a & b;
    int bit_or = a | b;
    int bit_xor = a ^ b;
    int bit_not = ~a;
    int shift_left = a << 2;
    int shift_right = b >> 1;
    
    // Unary operators
    int neg = -a;
    int pos = +a;
    int pre_inc = ++a;
    int post_inc = a++;
    int pre_dec = --b;
    int post_dec = b--;
    
    // Compound assignment
    a += 5;
    b -= 3;
    c *= 2;
    d /= 2;
    
    // Ternary operator
    int ternary = (a > b) ? a : b;
    
    // Comma operator
    int comma = (a = 1, b = 2, a + b);
    
    return comma;
}

// Pointer and address-of expressions
void test_pointers() {
    int x = 10;
    int* ptr = &x;
    int val = *ptr;
    
    int arr[5] = {1, 2, 3, 4, 5};
    int elem = arr[2];
    int* arr_ptr = arr + 1;
}

// Cast expressions
void test_casts() {
    double d = 3.14;
    int i = static_cast<int>(d);
    void* vp = reinterpret_cast<void*>(&i);
    const int& cr = const_cast<const int&>(i);
}
