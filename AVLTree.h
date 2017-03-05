#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <algorithm>
#include <functional>
#include <type_traits>
#include <memory>
#include <cassert>
#include <stdexcept>

template <typename ValueType> struct DefaultNodeBaseType
{
    DefaultNodeBaseType(const ValueType& ) {}
    void UpdateNodeState( const ValueType& v, DefaultNodeBaseType* p1, DefaultNodeBaseType* p2 ) {}
};

template <typename T, typename Comparer = std::less<T>, class NodeBaseType = DefaultNodeBaseType<T>> class AVLTree
{
public:
    typedef T ValueType;
    typedef Comparer ComparerType;

    class Node : public NodeBaseType 
    {
        friend class AVLTree;
    public:
        size_t GetHeight() const { return height; }
        const T& GetKey() const { return key; }
        const Node* GetChild( size_t i ) const { return children[i].get(); }
        const Node* GetParent() const { return parent; }        
    private:
        Node(const T& v) : NodeBaseType(v), height(1), key(v), parent(nullptr) {}
        
        void UpdateNodeState() {
            height = 1;
            size_t lh = 0, rh = 0;
            if( children[0] )
                lh = children[0]->height;
            if(children[1])
                rh = children[1]->height;
            height = 1 + std::max(lh, rh);
                
            NodeBaseType::UpdateNodeState( key, children[0].get(), children[1].get() );
        }
        
        size_t height;
        T key;
        Node* parent;
        std::unique_ptr<Node> children[2];
    };
   
private:
    std::unique_ptr<Node>* GetFromParentPointer(Node* p)
    {
        if( !p->parent )
            return &m_root;
        else if ( p->parent->children[0].get() == p )
            return &(p->parent->children[0]);
        else
            return &(p->parent->children[1]);
    }
    
    void CheckedSetParent( Node* child, Node* parent )
    {
        if( child )
            child->parent = parent;
    }
    
    AVLTree( std::unique_ptr<Node> p, const ComparerType& comp ) : m_comp(comp) { m_root.swap(p); }
    
    std::unique_ptr<Node> ExtractMax()
    {
        assert( m_root ); // tree must be non-empty
        Node* p = m_root.get();
        while( p->children[1] )
            p = p->children[1].get();
            
        std::unique_ptr<Node>* fromParent = GetFromParentPointer(p);
        Node* parent = p->parent;
        std::unique_ptr<Node> tmp;
        
        CheckedSetParent(p->children[0].get(), parent);
        
        tmp.swap( p->children[0] );
        tmp.swap( *fromParent );
        
        tmp->parent = nullptr;
        Rebalance(parent);
        
        return tmp;
    }
    
    void SetRootAndTwoSubtrees(
        std::unique_ptr<Node>& fromParent,
        std::unique_ptr<Node> newNode,
        std::unique_ptr<Node> child1,
        std::unique_ptr<Node> child2,
        Node* parent )
    {
        CheckedSetParent( child1.get(), newNode.get() );
        CheckedSetParent( child2.get(), newNode.get() );
        
        fromParent.swap( newNode );
        fromParent->parent = parent;
        
        fromParent->children[0].swap( child1 );
        fromParent->children[1].swap( child2 );
        fromParent->UpdateNodeState();
    }
    
    void MergeWithRoot( std::unique_ptr<Node> root, AVLTree& rightTree )
    {
        assert(root && !root->children[0] && !root->children[1]);
        
        size_t h1 = m_root ? m_root->height : 0;
        size_t h2 = rightTree.m_root ? rightTree.m_root->height : 0;
            
        if( h1 + 1 >= h2 && h1 <= h2 + 1 )
        {
            SetRootAndTwoSubtrees( m_root, move(root), move(m_root), move(rightTree.m_root), nullptr );
            return;
        }
        
        std::unique_ptr<Node>* treeToHang =     (h1 > h2) ? &(rightTree.m_root) : &m_root; // pointer to the smaller tree
        size_t dir =                            (h1 > h2) ? 1 : 0; // direction where to descend along the other tree
        size_t hMin =                           (h1 > h2) ? h2 : h1;
        Node* p =                               (h1 > h2) ? m_root.get() : rightTree.m_root.get();
        
        while( true )
        {
            size_t hNext = 0;
            if( p->children[dir] )
                hNext = p->children[dir]->height;
            if( hMin + 1 == hNext || hNext == hMin )
            {
                std::unique_ptr<Node> child1( dir == 0 ? move(*treeToHang) : move(p->children[dir]) );
                std::unique_ptr<Node> child2( dir == 1  ? move(*treeToHang) : move(p->children[dir]) );
                SetRootAndTwoSubtrees( p->children[dir], move(root), move(child1), move(child2), p );
                
                if( h1 < h2 )
                    m_root.swap( rightTree.m_root );

                Rebalance(p);
                return;
            }
            else
                p = p->children[dir].get();
        }
    }

    void AddChild( Node* p, size_t dir, const T& v )
    {
        assert( !p->children[dir] );
        p->children[dir].reset( new Node(v) );
        p->children[dir]->parent = p;
    }

    // moves the left and right subtrees into the separate trees and adds the node pointer to by p to the appropriate one
    // returns (p has a parent) AND (p is the left child)
    bool PrepareInitialSplittingState( Node* p, AVLTree& lt, AVLTree& rt, bool nodeGoesLeft )
    {
        lt.m_root.swap( p->children[0] );
        CheckedSetParent(lt.m_root.get(), nullptr);

        rt.m_root.swap( p->children[1] );
        CheckedSetParent(rt.m_root.get(), nullptr);

        std::unique_ptr<Node>* fromParent = GetFromParentPointer(p);
        assert( fromParent );       
        
        bool res = (p->parent && p == p->parent->children[0].get());
        
        p->parent = nullptr;
        std::unique_ptr<Node> tmp;
        tmp.swap( *fromParent );
        
        AVLTree midTree( move(tmp), GetComparer() );
        assert( midTree.m_root );
        midTree.m_root->UpdateNodeState();
        if( nodeGoesLeft )
        {
            lt.MergeWith(midTree);
        }
        else
        {
            midTree.MergeWith(rt);
            midTree.Swap(rt);
        }
        
        return res;
    }
    
    // performs one step of the splitting procedure
    // merges the remaining child of p with lt or rt, as appropriate
    // returns whether p is a left child of its parent
    bool DoOneSplittingStep( Node* p, bool isLeftChild, AVLTree& lt, AVLTree& rt )
    {
        std::unique_ptr<Node>* fromParent = GetFromParentPointer(p);
        assert(fromParent);
        bool res = (p->parent && p == p->parent->children[0].get());
        p->parent = nullptr;        
        
        size_t dir = isLeftChild ? 0 : 1;
        
        assert( !p->children[dir] ); // the child should have already been removed
        AVLTree mergedTree( GetComparer() );
        mergedTree.m_root.swap( p->children[1-dir] );
        CheckedSetParent(mergedTree.m_root.get(), nullptr);
        
        std::unique_ptr<Node> root;
        root.swap( *fromParent );
        root->UpdateNodeState();
        
        if( isLeftChild )            
            rt.MergeWithRoot(move(root), mergedTree);
        else
        {
            mergedTree.MergeWithRoot(move(root), lt);
            lt.Swap(mergedTree);
        }
        return res;
    }

    void DeleteNoRightChild( std::unique_ptr<Node>* fromParent, Node* p )
    {
        std::unique_ptr<Node> tmp;
        tmp.swap( p->children[0] );
        tmp.swap( *fromParent );
        CheckedSetParent( (*fromParent).get(), p->parent );
            
        Rebalance(p->parent);
    }

    void DeleteNextIsImmediateChild( std::unique_ptr<Node>* fromParent, Node* p, Node* next )
    {
        std::unique_ptr<Node> tmp;
        tmp.swap(p->children[1]);
        fromParent->swap( tmp );
        next->children[0].swap(p->children[0]);
        CheckedSetParent(next->children[0].get(), next);
        next->parent = p->parent;
        Rebalance(next);    
    }

    void DeleteNextNotImmediateChild(std::unique_ptr<Node>* fromParent, Node* p, Node* next )
    {
        Node* nextParent = next->parent;
        std::unique_ptr<Node>* fromNextParent = GetFromParentPointer(next);                
                
        p->children[0].swap( next->children[0] );
        assert( !p->children[0] );
        CheckedSetParent(next->children[0].get(), next);
        // p and next have swapped their left children
     
        std::unique_ptr<Node> tmp;           
        tmp.swap(p->children[1]); // tmp now has the right child of p.
        assert(!p->children[1]);
                
        tmp.swap(next->children[1]);
        CheckedSetParent(next->children[1].get(), next);
        // next now has the right child of p

        tmp.swap(*fromNextParent);
        CheckedSetParent( (*fromNextParent).get(), nextParent );
        // the original parent of next now points to the right child of next                
                
        tmp.swap(*fromParent);
        next->parent = p->parent;
        
        Rebalance(nextParent);    
    }

public:
    AVLTree() : m_root(nullptr), m_comp() {}
    AVLTree( const Comparer& comp ) : m_root(nullptr), m_comp(comp) {}   
    
    const Comparer& GetComparer() const { return m_comp; }
    
    const Node* GetRoot() const { return m_root.get(); }
    Node* GetRoot() { return m_root.get(); }
    
    const Node* GetMin() const { return GetExtreme( 0 ); }
    Node* GetMin() { return GetExtreme(0); }
    
    const Node* GetMax() const { return GetExtreme(1); }
    Node* GetMax() { return GetExtreme(1); }
    
    const Node* GetNext( const Node* p ) const { return GetNextImpl( p, 1 ); }
    Node* GetNext( Node* p ) { return GetNextImpl(p,1); }
    
    const Node* GetPrev( const Node* p) const { return GetNextImpl( p, 0 ); }
    Node* GetPrev(Node* p) { return GetNextImpl( p, 0 ); }
    
    void Swap( AVLTree& other )
    {
        m_root.swap( other.m_root );
    }
    
    void MergeWith( AVLTree& t )
    {
        assert( !GetRoot() || !t.GetRoot() || GetMax()->GetKey() < t.GetMin()->GetKey());
        if( !m_root )
        {
            Swap(t);
            return;
        }
        
        MergeWithRoot(ExtractMax(), t);
    }
    
    AVLTree Split( Node* p, bool nodeGoesLeft = false )
    {
        if( !m_root )
            return AVLTree( GetComparer() );
        
        if( !p )
            throw std::invalid_argument( "Split: the p argument must be non-null" );
            
        AVLTree lt(GetComparer()), rt(GetComparer());
        Node* parent = p->parent;
        bool isLeftChild = PrepareInitialSplittingState( p, lt, rt, nodeGoesLeft );

        p = parent;

        while( p )
        {
            parent = p->parent;
            bool nextLeftChild = DoOneSplittingStep( p, isLeftChild, lt, rt );            
            isLeftChild = nextLeftChild;
            p = parent;
        }
        
        Swap(lt);
        return rt;
    }
    
    const Node* Find( const T& v ) const { return FindImpl( v, m_root.get() ); }
    Node* Find( const T& v ) { return FindImpl( v, m_root.get() ); }    
    
    bool Add( const T& v )
    {
        if( ! m_root )
        {
            m_root.reset( new Node(v) );
            return true;
        }
        
        Node* where = Find(v);
        if( m_comp(v, where->key) )            
            AddChild( where, 0, v );        
        else if( m_comp(where->key, v) )
            AddChild( where, 1, v );
        else // value already present
            return false;
            
        where->UpdateNodeState();       
        Rebalance( where->parent );
            
        return true;
    }

    bool Delete( const T& v )
    {
        Node* p = Find(v);
        if( !p || m_comp(v, p->key) || m_comp(p->key, v) )
            return false;
        
        std::unique_ptr<Node>* fromParent = GetFromParentPointer(p);
        
        if( p->children[1] )
        {
            Node* next = p->children[1].get();
            while( next->children[0] )
                next = next->children[0].get();
            
            if( next == p->children[1].get() )
                DeleteNextIsImmediateChild(fromParent, p, next);
            else
                DeleteNextNotImmediateChild(fromParent, p, next);
        }
        else
            DeleteNoRightChild( fromParent, p );
        
        return true;
    }

    template <class Functor>
    void VisitInOrder( Functor f ) const
    {
        VisitInOrderImpl( f, m_root.get() );
    }

    template <class Functor>
    void VisitPreOrder( Functor f ) const
    {
        VisitPreOrderImpl( f, m_root.get() );
    }

    template <class Functor>
    void VisitPostOrder( Functor f ) const
    {
        VisitPostOrderImpl( f, m_root.get() );
    }

private:
    Node* GetExtreme( size_t dir ) const
    {
        if( !m_root )
            return nullptr;
        
        Node* p = m_root.get();
        while( p->children[dir] )
            p = p->children[dir].get();
        
        return p;
    }
    
    Node* GetNextImpl(Node* cur, size_t dir) const
    {
        if( !cur )
            throw std::invalid_argument( "The handle does not represent a valid position within the tree." );
        
        Node* p = nullptr;
        if( cur->children[dir] )
        {
            p = cur->children[dir].get();
            while( p->children[1-dir] )
                p = p->children[1-dir].get();
        }
        else
        {
            bool found = false;
            p = cur;
            while( p && !found )
            {
                if( p->parent)
                {
                     if( p == p->parent->children[1-dir].get() )
                         found = true;
                }
                p = p->parent;
            }            
        }
        
        return p;
    }

    template <class Functor>
    void VisitInOrderImpl( Functor f, const Node* p ) const
    {
        if(!p)
            return;
        
        VisitInOrderImpl( f, p->children[0].get() );
        f( p->key );        
        VisitInOrderImpl( f, p->children[1].get() );
    }
    
    template <class Functor>
    void VisitPreOrderImpl( Functor f, const Node* p ) const
    {
        if(!p)
            return;
        
        f( p->key );
        VisitPreOrderImpl( f, p->children[0].get() );
        VisitPreOrderImpl( f, p->children[1].get() );
    }

    template <class Functor>
    void VisitPostOrderImpl( Functor f, const Node* p ) const
    {
        if(!p)
            return;
        
        VisitPostOrderImpl( f, p->children[0].get() );
        VisitPostOrderImpl( f, p->children[1].get() );
        f( p->key );
    }

    Node* RotateOnce( std::unique_ptr<Node>& p, size_t ic )
    {
        std::unique_ptr<Node> tmp;
        Node* oldParent = p->parent;
        Node* root = p.get();
        tmp.swap( p ); // tmp points to C
        root->children[ic]->children[1-ic].swap(tmp);
        root->parent = root->children[ic].get();
        // tmp points to E, C is the right child of A
        
        root->children[ic].swap(tmp);
        CheckedSetParent(root->children[ic].get(), root );
        // E is the left child of C, tmp points to A
        
        tmp.swap( p );
        p->parent = oldParent;
        // A is the child of original P's parent. tmp is null
        
        root->UpdateNodeState();
        return p.get();
    }
//                           C
//                 A                   B
//             D       E
//                    F G
    // ic is the direction of the taller subtree
    Node* DoRotations( Node* p, size_t ic ) // returns the new root of the subtree    
    {
        size_t hD = p->children[ic]->children[ic] ? p->children[ic]->children[ic]->height : 0;
        size_t hE = p->children[ic]->children[1-ic] ? p->children[ic]->children[1-ic]->height : 0;
        
        std::unique_ptr<Node>* fromParent = GetFromParentPointer(p);
        
        if( hD < hE )
            RotateOnce(p->children[ic], 1-ic);
            
        Node* nr = RotateOnce(*fromParent, ic);
        nr->UpdateNodeState();
        return nr;
    }

    void Rebalance( Node* p )
    {
        if(!p)
            return;
            
        size_t h[2] = {0};
        for( size_t i = 0; i < 2; ++i )
            if( p->children[i] )
                h[i] = p->children[i]->height;
        
        bool rotated = false;
        for( size_t i = 0; i < 2; ++i )
            if( h[i] > h[1-i] + 1 )
            {
                assert(h[i] == h[1-i] + 2);
                p = DoRotations( p, i );
                rotated = true;
            }
        
        if( !rotated )
            p->UpdateNodeState();
        
        Rebalance(p->parent);
    }

    Node* FindImpl( const T& v, Node* pNode ) const
    {
        if( pNode == nullptr )
            return pNode;
            
        if( m_comp( v, pNode->key) )
        {
            if( pNode->children[0] != nullptr )
                return FindImpl( v, pNode->children[0].get() );
            else
                return pNode;
        }        
        if( m_comp( pNode->key, v ) )
        {
            if( pNode->children[1] )
                return FindImpl( v, pNode->children[1].get() );
            else
                return pNode;
        }        
        return pNode;
    }
    
    std::unique_ptr<Node> m_root;
    Comparer m_comp;
};

#endif
