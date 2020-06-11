#!/usr/bin/env python3

import sys
import h5py
import os
import zipfile
import numpy
from urllib.request import urlretrieve

def download(src, dst):
    if not os.path.isdir("downloads"):
        os.mkdir("downloads")
    dstpath = os.path.join("downloads", dst)
    if not os.path.exists(dstpath):
        # TODO: should be atomic
        print('downloading %s -> %s...' % (src, dstpath), file=sys.stderr)
        urlretrieve(src, dstpath)
    return dstpath


def get_dataset_fn(dataset):
    if not os.path.exists('data'):
        os.mkdir('data')
    return os.path.join('data', '%s.hdf5' % dataset)


def get_dataset(which):
    hdf5_fn = get_dataset_fn(which)
    if os.path.exists(hdf5_fn):
        return hdf5_fn
    # TODO: This is where we should put the code to cache the datasets online
    if which in DATASETS:
        print("Creating dataset locally", file=sys.stderr)
        DATASETS[which](hdf5_fn)
    hdf5_f = h5py.File(hdf5_fn)
    return hdf5_fn

# Everything below this line is related to creating datasets

def write_output(vectors, fn, distance, version, point_type='float', count=100):
    import sklearn.preprocessing
    n = 0
    f = h5py.File(fn, 'w')
    g = f.create_group(str(version))
    g.attrs['distance'] = distance
    g.attrs['point_type'] = point_type
    g.attrs['version'] = version
    if distance == 'angular':
        vectors = sklearn.preprocessing.normalize(vectors, axis=1, norm='l2')
    g.create_dataset('vectors', (len(vectors), len(vectors[0])), dtype=vectors.dtype)[:] = vectors
    f.close()

def gnews(out_fn): 
    import gensim

    url = 'https://s3.amazonaws.com/dl4j-distribution/GoogleNews-vectors-negative300.bin.gz'
    fn = download(url, 'GoogleNews-vectors-negative300.bin.gz')
    print("Loading GNews vectors", file=sys.stderr)
    model = gensim.models.KeyedVectors.load_word2vec_format(fn, binary=True)
    X = []
    for word in model.vocab.keys():
        X.append(model[word])
    print("Writing output", file=sys.stderr)
    write_output(numpy.array(X), out_fn, 'angular', version = 1)

def glove2m(out_fn):
    import zipfile

    url = 'http://nlp.stanford.edu/data/glove.840B.300d.zip'
    fn = download(url, 'glove.840B.300d.zip')
    with zipfile.ZipFile(fn) as z:
        print('preparing %s' % out_fn, file=sys.stderr)
        z_fn = 'glove.840B.300d.txt'
        X = []
        for line in z.open(z_fn):
            v = [float(x) for x in line.strip().split()[1:]]
            X.append(numpy.array(v))
        write_output(numpy.array(X), out_fn, 'angular', version = 1)

def glove(out_fn, d):
    import zipfile

    url = 'http://nlp.stanford.edu/data/glove.twitter.27B.zip'
    fn = download(url, 'glove.twitter.27B.zip')
    with zipfile.ZipFile(fn) as z:
        print('preparing %s' % out_fn, file=sys.stderr)
        z_fn = 'glove.twitter.27B.%dd.txt' % d
        X = []
        for line in z.open(z_fn):
            v = [float(x) for x in line.strip().split()[1:]]
            X.append(numpy.array(v))
        write_output(numpy.array(X), out_fn, 'angular', version = 1)


def _load_texmex_vectors(f, n, k):
    import struct

    v = numpy.zeros((n, k))
    for i in range(n):
        f.read(4)  # ignore vec length
        v[i] = struct.unpack('f' * k, f.read(k*4))

    return v


def _get_irisa_matrix(t, fn):
    import struct
    m = t.getmember(fn)
    f = t.extractfile(m)
    k, = struct.unpack('i', f.read(4))
    n = m.size // (4 + 4*k)
    f.seek(0)
    return _load_texmex_vectors(f, n, k)


def sift(out_fn):
    import tarfile

    url = 'ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz'
    fn = download(url, 'sift.tar.tz')
    with tarfile.open(fn, 'r:gz') as t:
        train = _get_irisa_matrix(t, 'sift/sift_base.fvecs')
        # test = _get_irisa_matrix(t, 'sift/sift_query.fvecs')
        write_output(train, out_fn, 'euclidean', version = 1)


def gist(out_fn):
    import tarfile

    url = 'ftp://ftp.irisa.fr/local/texmex/corpus/gist.tar.gz'
    fn = download(url, 'gist.tar.tz')
    with tarfile.open(fn, 'r:gz') as t:
        train = _get_irisa_matrix(t, 'gist/gist_base.fvecs')
        # test = _get_irisa_matrix(t, 'gist/gist_query.fvecs')
        write_output(train, out_fn, 'euclidean', version = 1)


def _load_mnist_vectors(fn):
    import gzip
    import struct

    print('parsing vectors in %s...' % fn, file=sys.stderr)
    f = gzip.open(fn)
    type_code_info = {
        0x08: (1, "!B"),
        0x09: (1, "!b"),
        0x0B: (2, "!H"),
        0x0C: (4, "!I"),
        0x0D: (4, "!f"),
        0x0E: (8, "!d")
    }
    magic, type_code, dim_count = struct.unpack("!hBB", f.read(4))
    assert magic == 0
    assert type_code in type_code_info

    dimensions = [struct.unpack("!I", f.read(4))[0] for i in range(dim_count)]

    entry_count = dimensions[0]
    entry_size = numpy.product(dimensions[1:])

    b, format_string = type_code_info[type_code]
    vectors = []
    for i in range(entry_count):
        vectors.append([struct.unpack(format_string, f.read(b))[0] for j in range(entry_size)])
    return numpy.array(vectors)


def mnist(out_fn):
    fn = download('http://yann.lecun.com/exdb/mnist/train-images-idx3-ubyte.gz', 'mnist-train.gz')
    # download('http://yann.lecun.com/exdb/mnist/t10k-images-idx3-ubyte.gz', 'mnist-test.gz')
    train = _load_mnist_vectors(fn)
    # test = _load_mnist_vectors('mnist-test.gz')
    write_output(train, out_fn, 'euclidean', version = 1)


def fashion_mnist(out_fn):
    fn = download('http://fashion-mnist.s3-website.eu-central-1.amazonaws.com/train-images-idx3-ubyte.gz', 'fashion-mnist-train.gz')
    # download('http://fashion-mnist.s3-website.eu-central-1.amazonaws.com/t10k-images-idx3-ubyte.gz', 'fashion-mnist-test.gz')
    train = _load_mnist_vectors(fn)
    # _test = _load_mnist_vectors('fashion-mnist-test.gz')
    write_output(train, out_fn, 'euclidean', version = 1)


DATASETS = {
    'fashion-mnist-784-euclidean': fashion_mnist,
    'gist-960-euclidean': gist,
    'glove-25-angular': lambda out_fn: glove(out_fn, 25),
    'glove-50-angular': lambda out_fn: glove(out_fn, 50),
    'glove-100-angular': lambda out_fn: glove(out_fn, 100),
    'glove-200-angular': lambda out_fn: glove(out_fn, 200),
    'glove-2m-300-angular': lambda out_fn: glove2m(out_fn),
    'gnews-300-angular': lambda out_fn: gnews(out_fn),
    'mnist-784-euclidean': mnist,
    'sift-128-euclidean': sift,
}

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE: ./datasets.py dataset_name", file=sys.stderr)
    dataset = sys.argv[1]
    print(get_dataset(dataset))
