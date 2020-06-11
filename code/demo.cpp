#include "report.hpp"
#include "datasets.hpp"

int main(int argc, char **argv) {
  // Bring the database schema to the most up to date version
  db_setup();

  // Datasets are loaded _by name_, not by filename!
  // It works as follows: the C++ calls Python and reads its
  // standard output, which contains the path to the hdf5 file
  // and the most recent dataset version.
  // Along the way, if the file or the version didn't exist,
  // python creates them: each version is stored in a HDF5
  // group at the top level.
  //
  // With this information, C++ reads the dataset from HDF5.
  auto dataset = load("fashion-mnist-784-euclidean");
  auto first = dataset.get(0);
  size_t dim = first.size();
  std::cout << "dimensionality is " << dim << std::endl;
  for (size_t i=0; i<dim; i++) {
    std::cout << first[i] << " ";
  }
  std::cout << std::endl;

  // Just a dummy call
  record_result("test", 1, "algotest", 1, "params", 128);

  return 0;
}
