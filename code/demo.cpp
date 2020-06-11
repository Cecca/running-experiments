#include "report.hpp"

int main(int argc, char **argv) {
  // Bring the database schema to the most up to date version
  int rc = db_setup();

  record_result("test", 1, "algotest", 1, "params", 128);

  return rc;
}
