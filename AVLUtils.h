#ifndef AVL_UTILS_H
#define AVL_UTILS_H

#include "AVLTree.h"

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
};

template <typename ValueType, typename SummationType=ValueType> class SumNodeBaseType
{
public:
    typedef SummationType SumType;

    SumNodeBaseType(const ValueType& v) : sum(v) {}
    
    const SumType& GetSum() const { return sum; }
    
    void UpdateNodeState( const ValueType& v, const SumNodeBaseType* p1, const SumNodeBaseType* p2 )
    {
        sum = v;
        if( p1 )
            sum += p1->key;
        if( p2 )
            sum += p2->key;
    }
    
private:
    SumType sum;
};

    
template <typename AVLTreeInstantiation>
typename AVLTreeInstantiation::Node::SumType GetRangeSumImpl(
    const typename AVLTreeInstantiation::Node* p,
    bool knowMin,
    const typename AVLTreeInstantiation::ValueType& minV,
    bool knowMax,
    const typename AVLTreeInstantiation::ValueType& maxV,
    const typename AVLTreeInstantiation::ValueType& l,
    const typename AVLTreeInstantiation::ValueType& h,
    const typename AVLTreeInstantiation::ComparerType& comp)
{
    using ST = typename AVLTreeInstantiation::Node::SumType;
    if(!p)
        return ST();
        
    if( knowMin && knowMax && !comp(minV, l) && !comp(h, maxV) )
        return p->sum;
        
    ST res = ST();
    
    if( !comp(p->GetKey(), l) && !comp(h, p->GetKey()) )
        res += p->GetKey();
    if( (!knowMin && comp(l, p->GetKey())) || !comp( min(p->GetKey(), h, comp), max(minV, l, comp) ) )
        res += GetRangeSumImpl<AVLTreeInstantiation>( p->GetChild(0), knowMin, minV, true, p->GetKey(), l, h, comp );
    if( (!knowMax && comp(p->GetKey(), h)) || !comp( min( maxV, h, comp ), max(p->GetKey(), l, comp) ) )
        res += GetRangeSumImpl<AVLTreeInstantiation>( p->GetChild(1), true, p->GetKey(), knowMax, maxV, l, h, comp );
    
    return res;
}

template <typename AVLTreeInstantiation>
typename AVLTreeInstantiation::Node::SumType GetRangeSum(
    const AVLTreeInstantiation& t,
    const typename AVLTreeInstantiation::ValueType& lb,
    const typename AVLTreeInstantiation::ValueType& ub)
{
    return GetRangeSumImpl<AVLTreeInstantiation>( t.GetRoot(), false, lb, false, ub, lb, ub, t.GetComparer() );
}

template <typename AVLTreeInstantiation>
const typename AVLTreeInstantiation::Node* GetNthSmallest( const AVLTreeInstantiation& t, size_t i )
{
    return GetNthSmallestImpl( t.GetRoot(), i );
}

template <typename AVLNode>
const AVLNode* GetNthSmallestImpl( const AVLNode* p, size_t i )
{
    
    if( !p || i >= p->GetSize() )
        throw std::invalid_argument( "Index out of range" );
    
    size_t ls = 0;
    if( p->GetChild(0) )
        ls = p->GetChild(0)->GetSize();
        
    if( i < ls )
        return GetNthSmallestImpl( p->GetChild(0), i);
    if( i == ls )
        return p;
    else
        return GetNthSmallestImpl( p->GetChild(1), i - ls -1 );
}

#endif
