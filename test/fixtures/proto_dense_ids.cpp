#include <list>

using namespace std;

namespace proto_dense_id_regression {
class Container {
public:
  struct Item;
  struct Entry {
    list<Item>::iterator item;
  };
  struct Item {
    int value;
  };
};
} // namespace proto_dense_id_regression
