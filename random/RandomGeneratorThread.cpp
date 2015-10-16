#include "RandomGeneratorThread.h"

namespace cpputils {

    RandomGeneratorThread::RandomGeneratorThread(ThreadsafeRandomDataBuffer *buffer, size_t minSize, size_t maxSize)
            : _randomGenerator(), _buffer(buffer), _minSize(minSize), _maxSize(maxSize) {
        ASSERT(_maxSize >= _minSize, "Invalid parameters");
    }

    void RandomGeneratorThread::loopIteration() {
        _buffer->waitUntilSizeIsLessThan(_minSize);
        size_t neededRandomDataSize = _maxSize - _buffer->size();
        ASSERT(_maxSize > _buffer->size(), "This could theoretically fail if another thread refilled the buffer. But we should be the only refilling thread.");
        Data randomData = _generateRandomData(neededRandomDataSize);
        _buffer->add(std::move(randomData));
    }

    Data RandomGeneratorThread::_generateRandomData(size_t size) {
        Data newRandom(size);
        _randomGenerator.GenerateBlock(static_cast<byte*>(newRandom.data()), size);
        return newRandom;
    }

}
