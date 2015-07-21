#include "DataInnerNode.h"
#include "DataLeafNode.h"
#include "DataNodeStore.h"
#include "messmer/blockstore/interface/BlockStore.h"
#include "messmer/blockstore/interface/Block.h"
#include "messmer/blockstore/utils/BlockStoreUtils.h"


using blockstore::BlockStore;
using blockstore::Block;
using blockstore::Key;
using cpputils::Data;
using cpputils::unique_ref;
using cpputils::make_unique_ref;
using std::runtime_error;
using boost::optional;
using boost::none;

namespace blobstore {
namespace onblocks {
namespace datanodestore {

DataNodeStore::DataNodeStore(unique_ref<BlockStore> blockstore, uint32_t blocksizeBytes)
: _blockstore(std::move(blockstore)), _layout(blocksizeBytes) {
}

DataNodeStore::~DataNodeStore() {
}

unique_ref<DataNode> DataNodeStore::load(unique_ref<Block> block) {
  assert(block->size() == _layout.blocksizeBytes());
  DataNodeView node(std::move(block));

  if (node.Depth() == 0) {
    return make_unique_ref<DataLeafNode>(std::move(node));
  } else if (node.Depth() <= MAX_DEPTH) {
    return make_unique_ref<DataInnerNode>(std::move(node));
  } else {
    throw runtime_error("Tree is to deep. Data corruption?");
  }
}

unique_ref<DataInnerNode> DataNodeStore::createNewInnerNode(const DataNode &first_child) {
  assert(first_child.node().layout().blocksizeBytes() == _layout.blocksizeBytes());  // This might be violated if source is from a different DataNodeStore
  //TODO Initialize block and then create it in the blockstore - this is more efficient than creating it and then writing to it
  auto block = _blockstore->create(Data(_layout.blocksizeBytes()).FillWithZeroes());
  return DataInnerNode::InitializeNewNode(std::move(block), first_child);
}

unique_ref<DataLeafNode> DataNodeStore::createNewLeafNode() {
  //TODO Initialize block and then create it in the blockstore - this is more efficient than creating it and then writing to it
  auto block = _blockstore->create(Data(_layout.blocksizeBytes()).FillWithZeroes());
  return DataLeafNode::InitializeNewNode(std::move(block));
}

optional<unique_ref<DataNode>> DataNodeStore::load(const Key &key) {
  auto block = _blockstore->load(key);
  if (block == none) {
    return none;
  } else {
    return load(std::move(*block));
  }
}

unique_ref<DataNode> DataNodeStore::createNewNodeAsCopyFrom(const DataNode &source) {
  assert(source.node().layout().blocksizeBytes() == _layout.blocksizeBytes());  // This might be violated if source is from a different DataNodeStore
  auto newBlock = blockstore::utils::copyToNewBlock(_blockstore.get(), source.node().block());
  return load(std::move(newBlock));
}

unique_ref<DataNode> DataNodeStore::overwriteNodeWith(unique_ref<DataNode> target, const DataNode &source) {
  assert(target->node().layout().blocksizeBytes() == _layout.blocksizeBytes());
  assert(source.node().layout().blocksizeBytes() == _layout.blocksizeBytes());
  Key key = target->key();
  {
    auto targetBlock = target->node().releaseBlock();
    cpputils::to_unique_ptr(std::move(target)).reset(); // Call destructor
    blockstore::utils::copyTo(targetBlock.get(), source.node().block());
  }
  auto loaded = load(key);
  assert(loaded != none);
  return std::move(*loaded);
}

void DataNodeStore::remove(unique_ref<DataNode> node) {
  auto block = node->node().releaseBlock();
  cpputils::to_unique_ptr(std::move(node)).reset(); // Call destructor
  _blockstore->remove(std::move(block));
}

uint64_t DataNodeStore::numNodes() const {
  return _blockstore->numBlocks();
}

void DataNodeStore::removeSubtree(unique_ref<DataNode> node) {
  DataInnerNode *inner = dynamic_cast<DataInnerNode*>(node.get());
  if (inner != nullptr) {
    for (uint32_t i = 0; i < inner->numChildren(); ++i) {
      auto child = load(inner->getChild(i)->key());
      assert(child != none);
      removeSubtree(std::move(*child));
    }
  }
  remove(std::move(node));
}

DataNodeLayout DataNodeStore::layout() const {
  return _layout;
}

}
}
}