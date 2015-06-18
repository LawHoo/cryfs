#include <google/gtest/gtest.h>

#include "../../../../interface/BlobStore.h"

class BlobStoreTest: public ::testing::Test {
public:
  BlobStoreTest();

  static constexpr uint32_t BLOCKSIZE_BYTES = 4096;

  std::unique_ptr<blobstore::BlobStore> blobStore;

  cpputils::unique_ref<blobstore::Blob> loadBlob(const blockstore::Key &key) {
    auto loaded = blobStore->load(key);
    EXPECT_TRUE((bool)loaded);
    return std::move(*loaded);
  }

  void reset(cpputils::unique_ref<blobstore::Blob> ref) {
    UNUSED(ref);
    //ref is moved into here and then destructed
  }
};
