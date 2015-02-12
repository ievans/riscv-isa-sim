// http://stackoverflow.com/questions/8480640/how-to-throw-a-c-exception
#include <stdexcept>

int compare(int a, int b) {
  if (a < 0 || b < 0) {
    throw std::invalid_argument("a or b negative");
  }
}

void foo() {
  try {
    compare(-1, 0);
  } catch (const std::invalid_argument& e) {
    // ...
  }
}

int main() {
  foo();
}
