#include "report.hpp"
#include "datasets.hpp"

int main(int argc, char **argv) {
  // Bring the database schema to the most up to date version
  db_setup();

  // Just a dummy call
  // record_result("test", 1, "algotest", 1, "params", 128);
  auto dataset = load("fashion-mnist-784-euclidean");
  for (size_t i=0; i<784; i++) {
    std::cout << dataset[i] << " ";
  }
  std::cout << std::endl;

  return 0;
}
