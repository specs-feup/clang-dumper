typedef unsigned long threshold_word;

struct __attribute__((aligned(16))) threshold_outer {
    struct threshold_nested {};
    threshold_word value;
};
