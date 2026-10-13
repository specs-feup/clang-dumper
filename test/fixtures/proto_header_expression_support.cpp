template <class... Ts>
struct PackedTypes {};

using PackedIntFloat = PackedTypes<int, float>;

template <class T>
struct ValueBase {
  static int value;
};

template <class T>
struct ValueDerived : ValueBase<T> {
  using ValueBase<T>::value;
};

template <class... Ts>
auto unary_left_fold(Ts... values) {
  return (values - ...);
}

template <class... Ts>
auto unary_right_fold(Ts... values) {
  return (... - values);
}

template <class... Ts>
auto binary_left_fold(Ts... values) {
  return (5 - ... - values);
}

template <class... Ts>
auto binary_right_fold(Ts... values) {
  return (values - ... - 5);
}

struct Addable {};
Addable operator+(Addable, Addable);

template <class... Ts>
auto overloaded_fold(Ts... values) {
  return (values + ...);
}

void instantiate_folds() {
  (void)unary_left_fold(1, 2);
  (void)unary_right_fold(1, 2);
  (void)binary_left_fold(1, 2);
  (void)binary_right_fold(1, 2);
  (void)overloaded_fold(Addable{}, Addable{});
}

template <class T, T... Values>
struct IntegerSequence {};

using IntegerSequenceThree = __make_integer_seq<IntegerSequence, int, 3>;
using PackedFloat = __type_pack_element<1, int, float>;

struct ThreeWayLeft {
  int value;
};

struct ThreeWayRight {
  int value;
};

int operator<=>(ThreeWayRight right, ThreeWayLeft left) {
  return left.value - right.value;
}

bool rewritten_less(ThreeWayLeft left, ThreeWayRight right) {
  return left < right;
}

bool rewritten_greater(ThreeWayLeft left, ThreeWayRight right) {
  return left > right;
}

struct LifetimeType {
  int value;
};

void lifetime_extension() {
  const LifetimeType &extended = LifetimeType{7};
  (void)extended;
}
