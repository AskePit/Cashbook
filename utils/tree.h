/*! @file
 *
 * Generic tree class.
 *
 */

#ifndef ASKELIB_STD_TREE_H
#define ASKELIB_STD_TREE_H

#include <list>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>

#define NodeList std::list<Node<T> *>

template <class T>
class Node
{
public:
    T data;
    Node *parent {nullptr};
    NodeList children;

    Node()
        : parent(nullptr)
        , m_ownedData(true)
    {}

    Node(const T &d, Node *p = nullptr, bool ownedData = true)
        : data(d)
        , parent(p)
        , m_ownedData(ownedData)
    {}

    // copied nodes are detached from parent
    Node(const Node &other)
        : data(other.data)
        , parent(nullptr)
        , m_ownedData(other.m_ownedData)
    {
        for(const auto *child : other.children) {
            auto node = new Node(*child);
            node->parent = this;
            children.push_back(node);
        }
    }

    Node<T> *clone() {
        return new Node(*this);
    }

    friend void swap(Node &n1, Node &n2)
    {
        std::swap(n1.data, n2.data);
        std::swap(n1.parent, n2.parent);
        std::swap(n1.children, n2.children);
        std::swap(n1.m_ownedData, n2.m_ownedData);
    }

    Node &operator=(Node other) {
        swap(*this, other);
        return *this;
    }

    Node(Node &&other)
        : Node()
    {
        swap(*this, other);
    }

    virtual ~Node() {
        /*if(m_ownedData && std::is_pointer<T>::value) {
            if(data) {
                delete data;
            }
        }*/

        for(auto *child : children) {
            delete child;
        }
    }

    Node *at(size_t index) {
        return *std::next(children.begin(), index);
    }

    const Node *at(size_t index) const {
        return *std::next(children.cbegin(), index);
    }

    Node *child(const T &data) {
        for(const auto child : children) {
            if(child->data == data) {
                return child;
            }
        }
        return nullptr;
    }

    const Node *child(const T &data) const {
        for(const auto child : children) {
            if(child->data == data) {
                return child;
            }
        }
        return nullptr;
    }

    bool isLeaf() const {
        return children.empty();
    }

    size_t childCount() const {
        return children.size();
    }

    Node *addChild(const T &data) {
        auto node = new Node(data, this);
        children.push_back(node);
        return node;
    }

    Node *addChildAt(const T &data, size_t position) {
        auto node = new Node(data, this);
        if(position < children.size()) {
            children.insert(std::next(children.cbegin(), position), node);
        } else {
            addChild(data);
        }

        return node;
    }

    void removeChild(const T &data) {
        auto it = std::find_if(children.cbegin(), children.cend(), [&data](const Node<T> *node) {
            return node->data == data;
        });
        if(it != children.end()) {
            Node *node = *it;
            children.erase(it);
            delete node;
        }
    }

    void removeChildAt(size_t position) {
        auto it = std::next( children.cbegin(), position );
        Node *node = *it;
        children.erase(it);
        delete node;
    }

    void detach() {
        if(!parent) {
            return;
        }

        auto &v = parent->children;
        v.erase(
            std::remove_if(v.begin(), v.end(), [this](const Node<T> *el) {
                return el->data == data;
            }), v.end()
        );

        parent = nullptr;
    }

    void attachChildToBegin(Node *child) {
        child->detach();
        child->parent = this;
        children.insert(children.begin(), child);
    }

    void attachChildToEnd(Node *child) {
        child->detach();
        child->parent = this;
        children.push_back(child);
    }

    void attachChildBefore(Node *child, Node *before) {
        child->detach();
        child->parent = this;

        const auto it = children.begin();
        while(it != children.end()) {
            if(*it == before) {
                children.insert(it, child);
                return;
            }
            ++it;
        }

        children.push_back(child);
    }

    void attachChildBefore(Node *child, T data) {
        child->detach();
        child->parent = this;

        const auto it = children.begin();
        while(it != children.end()) {
            if((*it)->data == data) {
                children.insert(it, child);
                return;
            }
            ++it;
        }

        children.push_back(child);
    }

    void attachChildAfter(Node *child, Node *after) {
        child->detach();
        child->parent = this;

        auto it = children.begin();
        while(it != children.end()) {
            if(*it == after) {
                if(it == std::prev(children.end())) {
                    children.push_back(child);
                    return;
                }

                children.insert(std::next(it), child);
                return;
            }
            ++it;
        }

        children.push_back(child);
    }

    void attachChildAfter(Node *child, T data) {
        child->detach();
        child->parent = this;

        const auto it = children.begin();
        while(it != children.end()) {
            if((*it)->data == data) {
                if(it == std::prev(children.end())) {
                    children.push_back(child);
                    return;
                }

                children.insert(std::next(it), child);
                return;
            }
            ++it;
        }

        children.push_back(child);
    }

    void attachChildAt(Node *child, size_t index) {
        child->detach();
        child->parent = this;
        children.insert(std::next(children.begin(), index), child);
    }

    void attachSelfBefore(Node *before) {
        before->parent->attachChildBefore(this, before);
    }

    void attachSelfAfter(Node *after) {
        after->parent->attachChildAfter(this, after);
    }

    void attachSelfAsChildToBegin(Node *parent) {
        parent->attachChildToBegin(this);
    }

    void attachSelfAsChildToEnd(Node *parent) {
        parent->attachChildToEnd(this);
    }

    void attachSelfAsChildBefore(Node *parent, Node *before) {
        parent->attachChildBefore(this, before);
    }

    void attachSelfAsChildBefore(Node *parent, T data) {
        parent->attachChildBefore(this, data);
    }

    void attachSelfAsChildAfter(Node *parent, Node *after) {
        parent->attachChildAfter(this, after);
    }

    void attachSelfAsChildAfter(Node *parent, T data) {
        parent->attachChildAfter(this, data);
    }

    void attachSelfAsChildAt(Node *parent, size_t index) {
        parent->attachChildAt(this, index);
    }

    std::vector<Node<T> *> toList() {
        std::vector<Node<T> *> v;
        v.push_back(this);

        for(auto *child : children) {
            auto childList = child->toList();
            v.insert(v.end(), childList.begin(), childList.end());
        }

        return v;
    }

    std::vector<Node<T> *> getLeafs() {
        std::vector<Node<T> *> v;
        if(isLeaf()) {
            v.push_back(this);
        }

        for(auto *child : children) {
            auto childList = child->getLeafs();
            v.insert(v.end(), childList.begin(), childList.end());
        }

        return v;
    }

private:
    bool m_ownedData {true};
};

template <class T>
using Tree = Node<T>;

#endif // ASKELIB_STD_TREE_H
