template <class T>
struct GuideBox {
  GuideBox(T);
  GuideBox(const GuideBox &);
};

template <class T>
explicit(true) GuideBox(T *) -> GuideBox<T>;

template <class T>
explicit(false) GuideBox(T &) -> GuideBox<T>;

template <class T>
  requires(sizeof(T) > 0)
GuideBox(T &&) -> GuideBox<T>;

GuideBox deduced_guide_box(1);

template <class T>
struct AggregateBox {
  T value;
};

AggregateBox deduced_aggregate_box{1};
