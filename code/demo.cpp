#include "report.hpp"
#include "datasets.hpp"

int main(int argc, char **argv) {
  // Bring the database schema to the most up to date version
  db_setup();

  // Just a dummy call
  // record_result("test", 1, "algotest", 1, "params", 128);
  std::cout << dataset_path("fashion-mnist-784-euclidean") << std::endl;

  return 0;
}
