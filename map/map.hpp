/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <cstddef>
#include <functional>

#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {

struct my_true_type {};
struct my_false_type {};

template <class T>
struct my_type_traits {
  using difference_type = typename T::difference_type;
  using value_type = typename T::value_type;
  using pointer = typename T::pointer;
  using reference = typename T::reference;
  using iterator_category = typename T::iterator_category;
  using iterator_assignable = typename T::iterator_assignable;
};

template <class Key, class T, class Compare = std::less<Key> >
class map {
 public:
  /**
   * the internal type of data.
   * it should have a default constructor, a copy constructor.
   * You can use sjtu::map as value_type by typedef.
   */
  using value_type = pair<const Key, T>;

 private:
  Compare lt;
  enum Colors { RED, BLACK };
  struct Node {
    value_type *val;
    Node *p{nullptr}, *ch[2]{nullptr, nullptr};  // 左（0）右（1）孩子。
    Colors color;                                // 节点颜色。

    Node() : val{nullptr} {}
    Node(value_type val, Colors color = RED, Node *ptr = nullptr)
        : val{new value_type{val}}, p{ptr}, color{color} {}
    Node(const Node &x) : p{p}, ch{x.ch[0], x.ch[1]}, color{x.color} {
      val = x.val ? new value_type{*x.val} : nullptr;
    }
    Node &operator=(const Node &x) {
      if (this != &x) {
        p = x.p, ch[0] = x.ch[0], ch[1] = x.ch[1], color = x.color;
        if (val) delete val;
        val = x.val ? new value_type{*x.val} : nullptr;
      }
      return *this;
    }
    // 判断左、右孩子。
    bool Check() const { return p->ch[1] == this; }
    ~Node() {
      if (val) delete val;
    }
  } *head;  // 头节点，空。
  size_t siz{0};

  void Copy(Node *&o, const Node *rhs) {
    o = rhs->val ? new Node{*rhs->val, rhs->color} : new Node{};
    if (rhs->ch[0]) Copy(o->ch[0], rhs->ch[0]), o->ch[0]->p = o;
    if (rhs->ch[1]) Copy(o->ch[1], rhs->ch[1]), o->ch[1]->p = o;
  }
  void Clear(Node *&o) {
    if (!o) return;
    Clear(o->ch[0]), Clear(o->ch[1]);
    delete o, o = nullptr;
  }

  void Link(Node *x, bool s, Node *y) {  // 将 x 的 s 孩子设置为 y.
    x->ch[s] = y;
    if (y) y->p = x;
  }
  void Rotate(Node *x, const bool &s) {
    Node *y = x->ch[s], *fa = x->p;
    Link(x, s, y->ch[s ^ 1]);
    Link(fa, x->Check(), y);
    Link(y, s ^ 1, x);
    std::swap(x->color, y->color);  // 调整颜色。
  }

  // 判断一个点的颜色（注意空节点为黑色）。
  // 推论：红点一定非空。
  friend Colors Color(Node *at) {
    return at && at->color == RED ? RED : BLACK;
  }
  Node *Find(const Key &x) const {
    Node *at = head->ch[0];
    while (at && (lt(at->val->first, x) || lt(x, at->val->first)))
      at = at->ch[lt(at->val->first, x)];
    return at ? at : head;
  }
  friend Node *Next(const Node *at) {
    if (at->ch[1])
      for (at = at->ch[1]; at->ch[0]; at = at->ch[0]);
    else {
      for (; at->p && at->Check(); at = at->p);
      at = at->p;
    }
    return const_cast<Node *>(at);
  }
  friend Node *Pre(const Node *at) {
    if (at->ch[0])
      for (at = at->ch[0]; at->ch[1]; at = at->ch[1]);
    else {
      for (; at->p && !at->Check(); at = at->p);
      at = at->p;
    }
    return const_cast<Node *>(at);
  }

  void InsertAdjust(Node *at) {
    while (Color(at->p) == RED) {
      bool s = at->Check(), sp = at->p->Check();
      if (Color(at->p->p->ch[sp ^ 1]) == RED) {
        at->p->p->color = RED;
        at->p->color = at->p->p->ch[sp ^ 1]->color = BLACK;
        at = at->p->p;
      } else {
        if (s != sp)
          at = at->p, Rotate(at, s);  // 改 at 是为了可以共用下面的代码。
        Rotate(at->p->p, sp);
      }
    }
  }
  void EraseAdjust(Node *at) {
    // 当 at 为双重黑节点时需要调整。
    // 头节点和虚兄弟的颜色已分别提前设为红、黑，可以跳入第二种情况。
    while (Color(at) == BLACK) {
      bool s = at->Check();
      Node *bro = at->p->ch[s ^ 1];
      if (Color(bro) == RED)  // 转为下一种情况。
        Rotate(at->p, s ^ 1), bro = at->p->ch[s ^ 1];
      if (Color(bro->ch[0]) == BLACK && Color(bro->ch[1]) == BLACK) {
        // 之前其它部分有 bug，由红黑树性质可以证明 bro 此时一定非空。
        bro->color = RED, at = at->p;  // 若 p 为红，一重黑转到 p 上。
      } else {
        if (Color(bro->ch[s]) == RED)  // 将红节点转到外侧变为下一种情况。
          Rotate(bro, s), bro = at->p->ch[s ^ 1];
        bro->ch[s ^ 1]->color = BLACK;
        Rotate(at->p, s ^ 1);
        break;
      }
    }
    at->color = BLACK;
  }

 public:
  /**
   * see BidirectionalIterator at CppReference for help.
   *
   * if there is anything wrong throw invalid_iterator.
   *     like it = map.begin(); --it;
   *       or it = map.end(); ++end();
   */
  class const_iterator;
  class iterator {
    friend class map;
    map *source{nullptr};
    Node *at{nullptr};

   public:
    // The following code is written for the C++ type_traits library.
    // Type traits is a C++ feature for describing certain properties of a type.
    // For instance, for an iterator, iterator::value_type is the type that the
    // iterator points to.
    // STL algorithms and containers may use these type_traits (e.g. the
    // following typedef) to work properly. See these websites for more
    // information: https://en.cppreference.com/w/cpp/header/type_traits About
    // value_type: https://blog.csdn.net/u014299153/article/details/72419713
    // About iterator_category: https://en.cppreference.com/w/cpp/iterator
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
    using iterator_assignable = my_true_type;
    // If you are interested in type_traits, toy_traits_test provides a place to
    // practice. But the method used in that test is old and rarely used, so you
    // may explore on your own.
    // Notice: you may add some code in here and class const_iterator and
    // namespace sjtu to implement toy_traits_test, this part is only for bonus.

    iterator() = default;
    iterator(const iterator &other) : source{other.source}, at{other.at} {}
    iterator(map *source, Node *at) : source{source}, at{at} {}

    iterator operator++(int) {
      iterator tmp = *this;
      operator++();
      return tmp;
    }

    iterator &operator++() {
      Node *tmp = Next(at);
      if (tmp == source->head->ch[1]) throw invalid_iterator{};  // end() + 1
      at = tmp;
      return *this;
    }

    iterator operator--(int) {
      iterator tmp = *this;
      operator--();
      return tmp;
    }

    iterator &operator--() {
      Node *tmp = Pre(at);
      if (!tmp) throw invalid_iterator{};  // begin() - 1
      at = tmp;
      return *this;
    }
    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    map::value_type &operator*() const { return *at->val; }
    bool operator==(const iterator &rhs) const { return at == rhs.at; }
    bool operator==(const const_iterator &rhs) const { return at == rhs.at; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return at != rhs.at; }
    bool operator!=(const const_iterator &rhs) const { return at != rhs.at; }

    /**
     * for the support of it->first.
     * See
     * <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/>
     * for help.
     */
    map::value_type *operator->() const noexcept { return at->val; }
  };
  class const_iterator {
    // it should has similar member method as iterator.
    //  and it should be able to construct from an iterator.
    friend class map;
    const map *source{nullptr};
    const Node *at{nullptr};

   public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
    using iterator_assignable = my_false_type;

    const_iterator() = default;
    const_iterator(const const_iterator &other)
        : source{other.source}, at{other.at} {}
    const_iterator(const iterator &other)
        : source{other.source}, at{other.at} {}
    const_iterator(const map *source, const Node *at)
        : source{source}, at{at} {}
    // And other methods in iterator.
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      operator++();
      return tmp;
    }

    const_iterator &operator++() {
      const Node *tmp = Next(at);
      if (tmp == source->head->ch[1]) throw invalid_iterator{};  // end() + 1
      at = tmp;
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator tmp = *this;
      operator--();
      return tmp;
    }

    const_iterator &operator--() {
      const Node *tmp = Pre(at);
      if (!tmp) throw invalid_iterator{};  // begin() - 1
      at = tmp;
      return *this;
    }
    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    const map::value_type &operator*() const { return *at->val; }
    bool operator==(const iterator &rhs) const { return at == rhs.at; }
    bool operator==(const const_iterator &rhs) const { return at == rhs.at; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return at != rhs.at; }
    bool operator!=(const const_iterator &rhs) const { return at != rhs.at; }

    const map::value_type *operator->() const noexcept { return at->val; }
  };

  map() {
    head = new Node{};
    head->ch[1] = new Node{}, head->ch[1]->p = head;
    // 注意：这样写的时候 end() 为 head !!!
  }
  map(const map &other) : siz{other.siz} { Copy(head, other.head); }

  map &operator=(const map &other) {
    if (this != &other)
      siz = other.siz, Clear(head), Copy(head, other.head);
    return *this;
  }

  ~map() { Clear(head); }
  /**
   * access specified element with bounds checking
   * Returns a reference to the mapped value of the element with key equivalent
   * to key. If no such element exists, an exception of type
   * `index_out_of_bound'
   */
  T &at(const Key &key) {
    Node *tmp = Find(key);
    if (tmp == head) throw index_out_of_bound{};
    return tmp->val->second;
  }
  const T &at(const Key &key) const {
    const Node *tmp = Find(key);
    if (tmp == head) throw index_out_of_bound{};
    return tmp->val->second;
  }
  /**
   * access specified element
   * Returns a reference to the value that is mapped to a key equivalent to key,
   *   performing an insertion if such key does not already exist.
   */
  T &operator[](const Key &key) {
    Node *tmp = Find(key);
    if (tmp == head) return insert({key, T{}}).first.at->val->second;
    return tmp->val->second;
  }
  /**
   * behave like at() throw index_out_of_bound if such key does not exist.
   */
  const T &operator[](const Key &key) const { return at(key); }
  /**
   * return a iterator to the beginning
   */
  iterator begin() {
    Node *at = head;  // 为空的时候 begin() == end()
    while (at->ch[0]) at = at->ch[0];
    return {this, at};
  }
  const_iterator cbegin() const {
    Node *at = head;  // 为空的时候 begin() == end()
    while (at->ch[0]) at = at->ch[0];
    return {this, at};
  }
  /**
   * return a iterator to the end
   * in fact, it returns past-the-end.
   */
  iterator end() { return {this, head}; }
  const_iterator cend() const { return {this, head}; }
  /**
   * checks whether the container is empty
   * return true if empty, otherwise false.
   */
  bool empty() const { return !siz; }
  /**
   * returns the number of elements.
   */
  size_t size() const { return siz; }
  /**
   * clears the contents
   */
  void clear() { Clear(head->ch[0]), siz = 0; }
  /**
   * insert an element.
   * return a pair, the first of the pair is
   *   the iterator to the new element (or the element that prevented the
   * insertion), the second one is true if insert successfully, or false.
   */
  pair<iterator, bool> insert(const value_type &value) {
    Node *at = Find(value.first), *fa = head;
    if (at != head) return {{this, at}, 0};
    at = head->ch[0];
    head->color = BLACK, head->ch[1]->color = RED;  // 头节点的颜色自由。
    while (at)
      fa = at, at = at->ch[lt(at->val->first, value.first)];
    bool s = fa == head || lt(value.first, fa->val->first);
    fa->ch[s ^ 1] = at = new Node{value, RED, fa};
    // 新的叶子一定为红。
    ++siz, InsertAdjust(at);
    head->ch[0]->color = BLACK;
    return {{this, at}, 1};
  }
  /**
   * erase the element at pos.
   *
   * throw if pos pointed to a bad element (pos == this->end() || pos points an
   * element out of this)
   */
  void erase(iterator pos) {
    if (pos.at == head || pos.source != this) throw invalid_iterator{};
    Node *at = pos.at;
    head->color = RED, head->ch[1]->color = BLACK;  // 头节点的颜色自由。
    if (at->ch[0] && at->ch[1]) {
      Node *tmp = at->ch[1];
      while (tmp->ch[0]) tmp = tmp->ch[0];
      // 注意：不能仅交换值。
      std::swap(tmp->color, at->color);
      if (tmp->p == at) {
        Link(at->p, at->Check(), tmp);
        Link(at, 1, tmp->ch[1]);
        Link(tmp, 1, at);
      } else {
        std::swap(tmp->p->ch[0], at->p->ch[at->Check()]);
        std::swap(tmp->p, at->p);
        Node *x = tmp->ch[1];
        Link(tmp, 1, at->ch[1]);
        Link(at, 1, x);
      }
      tmp->ch[0] = at->ch[0], at->ch[0]->p = tmp, at->ch[0] = nullptr;
    }
    if (at->ch[0] || at->ch[1]) {
      bool s = at->ch[0] == nullptr;
      at->ch[s]->p = at->p, at->p->ch[at->Check()] = at->ch[s];
      at->ch[s]->color = BLACK;
    } else {
      EraseAdjust(at);
      // 可以证明，这里不需要单独调整根的颜色（T13.4-1）。
      at->p->ch[at->Check()] = nullptr;
    }
    delete at, --siz;
  }
  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   *   which is either 1 or 0
   *     since this container does not allow duplicates.
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key &key) const { return Find(key) != head; }
  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is
   * returned.
   */
  iterator find(const Key &key) { return {this, Find(key)}; }
  const_iterator find(const Key &key) const { return {this, Find(key)}; }
};

}  // namespace sjtu

#endif