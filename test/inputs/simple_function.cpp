// Test: Simple function declarations and definitions
// Tests basic FunctionDecl, ParmVarDecl, ReturnStmt, CompoundStmt

int add(int a, int b) {
    return a + b;
}

void empty_function() {
}

int no_params() {
    return 42;
}

// Forward declaration
void forward_declared();

// Function with default parameter
int with_default(int x, int y = 10) {
    return x + y;
}
