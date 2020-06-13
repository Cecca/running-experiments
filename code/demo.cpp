#include "report.hpp"
#include "datasets.hpp"
#include "toy_project/vector_storage.hpp"
#include "toy_project/real_vector.hpp"
#include "toy_project/distance.hpp"
#include "toy_project/unit_vector.hpp"
#include "toy_project/filter.hpp"

using namespace toy_project;

void show_usage(std::string name) {

    std::cerr << "Usage: " << name << " [OPTIONS]\n"
              << "\t--list_datasets List available datasets\n"
              << "\t--list_methods List available methods\n"
              << "\t--dataset NAME\n"
              << "\t--help Displays this text\n"
              << "\t--method simple | avx2 | avx512\n"
              << "\t--force Overwrite run if it's present\n"
              << std::endl;
}

template <typename T, typename D, typename F>
std::pair<long long, std::vector<size_t>> bruteforce(
        const VectorStorage<T>& dataset,
        const VectorStorage<T>& queryset,
        size_t dim,
        std::string method,
        F& filter
) {
    auto dist = D::simple_distance;
    if (method == "avx2") {
        dist = D::avx2_distance;
    } else if (method == "avx512") {
        dist = D::avx512_distance;
    }


    std::vector<size_t> res;

    auto start = std::chrono::high_resolution_clock::now();
    long long dist_comps = 0;

    for (size_t i=0; i < queryset.get_size();  i++) {
        auto query = queryset[i];
        float best_dist = D::inf(); 
        int best_index= -1;

        for (size_t j=0; j < dataset.get_size(); j++) {
            if (filter.passes(i, j)) {
                auto d = dist(query, dataset[j], dim);
                dist_comps++;
                if (D::cmp(d, best_dist)) {
                    best_dist = d;
                    filter.update(d);
                    best_index = j;
                }
            }
        }
        res.push_back(best_index);
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << (end-start).count() / 1e6 << std::endl;
    std::cout << dist_comps / ((float)dataset.get_size() * queryset.get_size()) << std::endl;
    return std::make_pair((end-start).count(), res);
}

template <typename T, typename D, typename F>
std::pair<long long, std::vector<size_t>>
run_experiment(Dataset& dataset, Dataset& queries,
        const std::string method, float recall) {

    auto dim = dataset.dimension();
    auto data_vectors = VectorStorage<T>(dim, dataset.num_vectors());
    auto query_vectors = VectorStorage<T>(dim, queries.num_vectors());

    for (int i=0; i < dataset.num_vectors(); i++) {
        data_vectors.insert(dataset.get(i));
    }

    for (int i=0; i < queries.num_vectors(); i++) {
        query_vectors.insert(queries.get(i));
    }

    F filter = F(recall);
    filter.setup(data_vectors, query_vectors);

    return bruteforce<T,D,F>(data_vectors, query_vectors, dim, method, filter);
}

int main(int argc, char **argv) {
  std::cout << "Running with code version " << GIT_REV << std::endl;

  // Bring the database schema to the most up to date version
  db_setup();

  // Simplified command line reading

  std::string dataset_name = "fashion-mnist-784-euclidean";
  std::string method = "simple";
  std::string storage = "float_aligned";
  bool filter = false;
  bool force = false;

  float recall = 0.5f;

  for (int i=1; i < argc; i++) {
      std::string arg = argv[i];
      if ((arg == "-h" || (arg == "--help"))) {
          show_usage(argv[0]);
          return 1;
      }
      if (arg == "--dataset") {
          if (i+1 < argc) {
              dataset_name = argv[++i];
          }
      }
      if (arg == "--method") {
          if (i+1 < argc) {
              method = argv[++i];
          }
      }
      if (arg == "--storage") {
          if (i+1 < argc) {
              storage = argv[++i];
          }
      }
      if (arg == "--recall") {
          if (i+1 < argc) {
              recall = atof(argv[++i]);
          }
      }
      if (arg == "--force") {
          force = true;
      }
      if (arg == "--filter") {
          filter = true;
      }
  }

  std::string run_identifier = "method=" + method + "; storage=" + storage;

  // Datasets are loaded _by name_, not by filename!
  // It works as follows: the C++ calls Python and reads its
  // standard output, which contains the path to the hdf5 file
  // and the most recent dataset version.
  // Along the way, if the file or the version didn't exist,
  // python creates them: each version is stored in a HDF5
  // group at the top level.
  //
  // With this information, C++ reads the dataset from HDF5.
  auto datasets = load(dataset_name);
  if (!force && contains_result(dataset_name, 1, "bruteforce", 1, run_identifier)) {
      std::cout << "Experiment already carried out -- skipping" << std::endl;
      return 0;
  }

  // TODO data might be aligned because of the dimensionality.
  if (storage.find("unaligned") != std::string::npos &&
          method.find("avx") != std::string::npos) {
      std::cerr << "Cannot run vectorized code on unaligned data. " << std::endl;
      return 3;
  }


  std::pair<long long, std::vector<size_t>> res;

  // transforming data into memory aligned storage
  if (dataset_name.find("euclidean") != std::string::npos) {
      if (storage == "float_aligned") {
          res = run_experiment<RealVectorFormat, L2, 
                    NoFilter<RealVectorFormat>>(
                            datasets.first, datasets.second, method, recall);
      }
  }
  else if (dataset_name.find("angular") != std::string::npos) {
      if (storage == "i16_aligned") {
          if (!filter) {
              res = run_experiment<UnitVectorFormat, IP_i16,
                      NoFilter<UnitVectorFormat>>(
                          datasets.first, datasets.second, method, recall);
          } else {
              res = run_experiment<UnitVectorFormat, IP_i16,
                      Filter<UnitVectorFormat>>(
                          datasets.first, datasets.second, method, recall);
          }
      } else if (storage == "float_aligned") {
          if (!filter) {
              res = run_experiment<RealVectorFormat, IP_float,
                        NoFilter<RealVectorFormat>>(
                                datasets.first, datasets.second, method, recall);
          } else {
              res = run_experiment<RealVectorFormat, IP_float,
                        Filter<RealVectorFormat>>(
                                datasets.first, datasets.second, method, recall);
          }
      }
  } else {
      std::cerr << "No valid distance function found." << std::endl;
      return 2;
  }

  record_result(dataset_name, 1, "bruteforce", 1, run_identifier, res.first);

  return 0;
}
