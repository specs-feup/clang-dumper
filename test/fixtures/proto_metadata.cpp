template <typename T> T dependent_construct() { return T(1); }
template <typename T> T dependent_list() { return T{1}; }

int cast_forms(double value) {
  int implicit = value;
  int explicit_c = (int)value;
  return implicit + explicit_c;
}

template <typename T> struct OutOfLineTemplate {
  template <typename U> void method(U);
};
template <typename T>
template <typename U>
void OutOfLineTemplate<T>::method(U) {}

template <typename T> struct Partial;
template <typename T> struct Partial<T *> { T value; };

struct MemberPointerOwner { int value; };
int MemberPointerOwner::*member_pointer = &MemberPointerOwner::value;

void lambda_captures(int value) {
  auto captures = [copy = value, direct{value}] {};
}
template <typename... Ts> auto pack_capture(Ts... values) {
  return [...copies = values] {};
}

void assembly_forms() {
  asm inline("nop");
  int value = 1;
  asm goto("test %0, %0\n\tjz %l[target]" : : "r"(value) : "cc" : target);
target:
  ;
}

template <typename T> struct FriendOwner {
  template <typename U> friend void friend_function(U);
  friend void declared_friend();
  friend struct FriendType;
};
