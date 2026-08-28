struct anonymous_union {
  union {
    int value;
  };
};

struct nested_anonymous_union {
  union {
    struct {
      int value;
    };
  };
};

struct anonymous_union first = { .value = 1 };
struct nested_anonymous_union second = { .value = 2 };
