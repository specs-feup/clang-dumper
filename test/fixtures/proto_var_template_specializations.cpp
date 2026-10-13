template <typename T, int N>
constexpr int variable_template = N;

template <typename T>
constexpr int variable_template<T *, 2> = 12;

template <>
constexpr int variable_template<int *, 2> = 34;

constexpr int partial_instantiation = variable_template<float *, 2>;
