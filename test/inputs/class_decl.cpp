// Test: Class declarations and member functions
// Tests CXXRecordDecl, CXXMethodDecl, FieldDecl, AccessSpecDecl

class SimpleClass {
public:
    int value;
    
    SimpleClass() : value(0) {}
    
    explicit SimpleClass(int v) : value(v) {}
    
    int getValue() const {
        return value;
    }
    
    void setValue(int v) {
        value = v;
    }
    
private:
    int privateField;
};

// Struct (public by default)
struct SimpleStruct {
    int x;
    int y;
    
    int sum() const {
        return x + y;
    }
};

// Inheritance
class Derived : public SimpleClass {
public:
    Derived() : SimpleClass(100) {}
    
    int getDoubleValue() const {
        return getValue() * 2;
    }
};
