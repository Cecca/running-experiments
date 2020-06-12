#include "report.hpp"
#include "datasets.hpp"
#include "toy_project/dataset.hpp"
#include "toy_project/real_vector.hpp"
#include "toy_project/distance.hpp"

using namespace toy_project;

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
  std::cout << "Number of vectors is " << dataset.num_vectors() << std::endl;
  auto first = dataset.get(0);
  size_t dim = first.size();
  std::cout << "dimensionality is " << dim << std::endl;
  for (size_t i=0; i<dim; i++) {
    std::cout << first[i] << " ";
  }
  std::cout << std::endl;

  // transforming data into memory aligned storage
  auto vector_storage =
      toy_project::Dataset<RealVectorFormat>(dim, dataset.num_vectors());

  for (size_t i=0; i < dim; i++) {
      vector_storage.insert(dataset.get(i));
  }

  const float* query = vector_storage[0];
  float min_dist = std::numeric_limits<float>::max();
  int min_index = -1;

  for (int i=1; i < dim; i++) {
      if (l2_distance_float_simple(query,
           vector_storage[i], dim) < min_dist) {
        min_dist = l2_distance_float_simple(query, vector_storage[i], dim);
        min_index = i;
      }
  }

 std::cout << min_index << std::endl;

  // Just a dummy call
  record_result("test", 1, "algotest", 1, "params", 128);

  return 0;
}
