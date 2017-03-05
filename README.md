# avl-tree
A C++ implementation of AVL trees with the possibility of maintaining additional properties (like subtree node count).

This is a small header-only library that provides the AvlTree class template along with two small classes (in AvlUtils.h) that serve as examples of how the AvlTree template can be customized.

## The AvlTree template
This is a class template for an [AVL tree](https://en.wikipedia.org/wiki/AVL_tree) holding unique elements. That is, if two elements are considered to be equal (i.e. neither is "less" than the other) then they can't both be in the same tree. This class template has the following 3 parameters.

Parameter | Description
--------- | -----------
T | The value type that should be stored in the nodes of the tree.
Comparer | A type that defines how the elements of the tree are ordered - i.e. the "less-than" relation. If not provided, defaults to std::less&lt;T&gt;.
NodeBaseType | A class that should serve as a base class for AvlTree::Node. Defaults to DefaultNodeBase<T> - an empty class that does not maintain any additional bookkeeping information.

On the lower level, the AvlTree::Node class is designed to allow read-only operations via the following public interface:
```cpp
  size_t GetHeight() const;
  const T&amp; GetKey() const;
  const Node* GetChild( size_t i ) const;
  const Node* GetParent() const;
```

The public interface of AvlTree is as follows:

Method | Description | Time Complexity
------ | ----------- | ---------------
AVLTree() | Default constructor
AVLTree( const Comparer&amp; comp ) | A constructor with provided Comparer instance
const Comparer&amp; GetComparer() const | Method got getting the comparer used in a particular AvlTreeInstance.
const Node* GetRoot() const | Gets a pointer to the const root. | O(1)
Node* GetRoot() const | Gets a pointer to the non-const root. As discussed above, the AvlTree::Node class does not have any methods that can modify a tree. However, some of AvlTree methods require pointers to non-const nodes (such as Split()). | O(1)
const Node* GetMin() const | Returns the pointer to the const node containing the minimal element of the tree. Returns `nullptr` if the tree is empty. | O(logN)
const Node* GetMin() const | Returns the pointer to the non-const node containing the minimal element of the tree. Returns `nullptr` if the tree is empty. | O(logN)
const Node* GetMax() const | Returns the pointer to the const node containing the maximal element of the tree. Returns `nullptr` if the tree is empty. | O(logN)
const Node* GetMax() const | Returns the pointer to the non-const node containing the maximal element of the tree. Returns `nullptr` if the tree is empty. | O(logN)
const Node* GetNext( const Node* p ) const | Returns a pointer to a const Node containing the smallest key that is greater than that of `p`. `nullptr` is returned if `p` contains the largest element overall. | O(logN) but traversal of the whole tree is O(N)
Node* GetNext( const Node* p ) | Returns a pointer to a non-const Node containing the smallest key that is greater than that of `p`. `nullptr` is returned if `p` contains the largest element overall. | O(logN) but traversal of the whole tree is O(N)
const Node* GetPrev( const Node* p ) const | Returns a pointer to a const Node containing the greatest key that is smaller than that of `p`. `nullptr` is returned if `p` contains the smallest element overall. | O(logN) but traversal of the whole tree is O(N)
Node* GetPrev( const Node* p ) | Returns a pointer to a non-const Node containing the greatest key that is smaller than that of `p`. `nullptr` is returned if `p` contains the smallest element overall. | O(logN) but traversal of the whole tree is O(N)
void Swap( AVLTree&amp; other ) | A method for swapping the trees of two AvlTree objects. **Warning: the Comparators are currently not swapped and not checked for equivalence.** | O(1)
void MergeWith( AVLTree&amp; t ) | Given a tree where all the values are greater than those contained in the current one merge that tree into the current tree. Tree `t` becomes empty as a result of this operation. **Warning: no checking is performed for whether all the keys of `this` are indeed smaller than those of `t`** | O(logN)
AVLTree Split( Node* p, bool nodeGoesLeft = false ) | Given a position in the current tree (defined by `p`) split it into two. The `nodeGoesLeft` parameter controls whether the Node pointer by `p` should belong to the "less-than" tree. As a result of this operation the current tree contains elements less than that of the node pointed to by `p`. The other nodes are moved to the tree that is returned by this method. | O(logN)
const Node* Find( const T&amp; v ) const | Returns a pointer to the const node containing value `v` if it exists and `nullptr` otherwise. | O(logN)
Node* Find( const T&amp; v ) | Returns a pointer to the non-const node containing value `v` if it exists and `nullptr` otherwise. | O(logN)
bool Add( const T&amp; v ) | Adds value `v` to the tree. The method returns true if the operation is successful (i.e. the value was not already in the tree) | O(logN)
bool Delete( const T&amp; v ) | Deletes value `v` from the tree. The method returns true if the operation is successful (i.e. if such value was found in the tree) | O(logN)
template &lt;class Functor&gt; void VisitInOrder( Functor f ) const | Applies functor `f` that should accept `const T&` to the nodes of the AvlTree in an in-order traversal. | O(N)
template &lt;class Functor&gt; void VisitPreOrder( Functor f ) const | Applies functor `f` that should accept `const T&` to the nodes of the AvlTree in a pre-order traversal. | O(N)
template &lt;class Functor&gt; void VisitPostOrder( Functor f ) const | Applies functor `f` that should accept `const T&` to the nodes of the AvlTree in a post-order traversal. | O(N)

## Customization by providing alternative base classes to AvlTree::Node
The AvlTree may maintain additional information for you if you provide an alternative base class for AvlTree::Node. The kind of information that can be maintained is limited to whatever can be propagated from the leaf nodes to the root. In other words if for every node you need to maintain property X that can be calculated given property X of the left and right child nodex and whatever information is available at the current node (the value of the key) then this property can be maintained correctly by the AvlTree. When implementing such a class you are asked to implement two methods:

1. A constructor,
2. A method that updates the state at a given node. This method is called at appropriate times when modificatios to an AVL tree happen (rotations, deletions etc.).

As an example, here is an extract from AvlUtils.h. It is the code of the base class that allows maintaining the number of nodes at every sub-tree.

```cpp
template <typename ValueType> class TreeSizeNodeBaseType
{
public:
    TreeSizeNodeBaseType(const ValueType&) : treeSize(1) {}
    size_t GetSize() const { return treeSize; }
    
    void UpdateNodeState( const ValueType&, TreeSizeNodeBaseType* p1, TreeSizeNodeBaseType* p2 )
    {
        treeSize = 1;
        if( p1 )
            treeSize += p1->treeSize;
        if( p2 )
            treeSize += p2->treeSize;
    }
private:
    size_t treeSize;
}
```

When you have `TreeSizeNodeBaseType` as the base class of AvlTree::Node you can do things like `tree.GetRoot()->GetSize()`. Obviously, maintaining the tree size is extra work and it is not done by default. This feature would be useful if you needed to find the i-th largest element in a tree in O(logN) time.

## Concluding remarks
This code expects the key comparison operation not to throw. If this condition is satisfied then we have a strong exception safety guarantee because the only places where we could have exceptions are:

* the Add() method. An exception would be thrown if we fail to allocate a new node. But the tree would stay unmodified.
* the Split() method. An exception can be thrown if we fail to copy-construct a Comparer. But again, this would happen before we try to modify the tree.

This code is not thread-safe.
