// RUN: %check_clang_tidy %s bihler-unsafe-allocation %t

#include <cstdint>
#include <new>

enum class blocksize_typ {
  Bytes16 = 4,
};

template <blocksize_typ blocksize>
class BihlHeap {
public:
  void *malloc(uint32_t) {
    throw std::bad_alloc();
  }

  static void *malloc(void *, uint32_t) {
    throw std::bad_alloc();
  }
};

template <blocksize_typ blocksize>
class BihlNovheap : public BihlHeap<blocksize> {
public:
  void *malloc(uint32_t, uint32_t) {
    throw std::bad_alloc();
  }

  void *NovElementResize(void *, uint32_t) {
    throw std::bad_alloc();
  }
};

void test_bihlheap_throwing_methods() {
  BihlHeap<blocksize_typ::Bytes16> heap;
  BihlNovheap<blocksize_typ::Bytes16> novheap;

  heap.malloc(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Bihl heap method '{{.*}}::malloc' is not protected by try-catch block and may throw std::bad_alloc [bihler-unsafe-allocation]

  BihlHeap<blocksize_typ::Bytes16>::malloc(&heap, 1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Bihl heap method '{{.*}}::malloc' is not protected by try-catch block and may throw std::bad_alloc [bihler-unsafe-allocation]

  novheap.malloc(1, 77);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Bihl heap method '{{.*}}::malloc' is not protected by try-catch block and may throw std::bad_alloc [bihler-unsafe-allocation]

  novheap.NovElementResize(nullptr, 2);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Bihl heap method '{{.*}}::NovElementResize' is not protected by try-catch block and may throw std::bad_alloc [bihler-unsafe-allocation]
}

void test_bihlheap_throwing_methods_in_try() {
  BihlHeap<blocksize_typ::Bytes16> heap;
  BihlNovheap<blocksize_typ::Bytes16> novheap;

  try {
    heap.malloc(1);
    BihlHeap<blocksize_typ::Bytes16>::malloc(&heap, 1);
    novheap.malloc(1, 77);
    novheap.NovElementResize(nullptr, 2);
  } catch (const std::bad_alloc &) {
  }
}
