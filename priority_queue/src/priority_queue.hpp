#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>

#include "exceptions.hpp"

namespace sjtu {

/**
 * a container like std::priority_queue which is a heap internal.
 */
template <typename T, class Compare = std::less<T>>
class priority_queue {
  struct Node {
    int dis{0};
    T *val;
    Node *ls{nullptr}, *rs{nullptr};

    Node() {}
    Node(const T &x, const int &dis = 0) : dis{dis} {
      val = new T{x};
    }
    bool operator<(const Node &rhs) const {
      return Compare{}(*val, *rhs.val);
    }
    ~Node() {
      if (val) delete val;
    }
  } *root;
  int siz;  // 堆的大小。

  void Copy(Node *&o, const Node *rhs) {
    o = new Node{*rhs->val, rhs->dis};
    if (rhs->ls) Copy(o->ls, rhs->ls);
    if (rhs->rs) Copy(o->rs, rhs->rs);
  }
  void Clear(Node *&o) {
    if (!o) return;
    Clear(o->ls), Clear(o->rs);
    delete o, o = nullptr;
  }
  Node *Merge(Node *o, Node *rhs) {
    if (!o || !rhs) return o ? o : rhs;
    if (*o < *rhs) std::swap(o, rhs);  // 大根堆。
    o->rs = Merge(o->rs, rhs);
    if (!o->ls || o->rs && o->ls->dis < o->rs->dis)
      std::swap(o->ls, o->rs);
    o->dis = o->rs ? o->rs->dis + 1 : 0;
    return o;
  }
 public:

  priority_queue() : root{nullptr}, siz{0} {}
  priority_queue(const priority_queue &other) : siz{other.siz} {
    Copy(root, other.root);
  }

  ~priority_queue() { Clear(root); }

  priority_queue &operator=(const priority_queue &other) {
    if (&other == this) return *this;
    siz = other.siz;
    Clear(root), Copy(root, other.root);
    return *this;
  }
  /**
   * get the top of the queue.
   * @return a reference of the top element.
   * throw container_is_empty if empty() returns true;
   */
  const T &top() const {
    if (!siz) throw container_is_empty();
    return *root->val;
  }
  /**
   * TODO
   * push new element to the priority queue.
   */
  void push(const T &e) {
    Node *p = new Node{e};
    root = Merge(root, p);
    ++siz;
  }
  /**
   * TODO
   * delete the top element.
   * throw container_is_empty if empty() returns true;
   */
  void pop() {
    if (!siz) throw container_is_empty();
    Node *tmp = root;
    root = Merge(tmp->ls, tmp->rs);
    delete tmp, --siz;
  }
  /**
   * return the number of the elements.
   */
  size_t size() const { return siz; }
  /**
   * check if the container has at least an element.
   * @return true if it is empty, false if it has at least an element.
   */
  bool empty() const { return !siz; }
  /**
   * merge two priority_queues with at least O(logn) complexity.
   * clear the other priority_queue.
   */
  void merge(priority_queue &other) {
    root = Merge(root, other.root), siz += other.siz;
    other.root = nullptr, other.siz = 0;
  }
};

}  // namespace sjtu

#endif