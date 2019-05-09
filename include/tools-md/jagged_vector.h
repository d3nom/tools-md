/*
MIT License

Copyright (c) 2019 Michel Dénommée

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _tools_md_jagged_vector_h
#define _tools_md_jagged_vector_h

#include "stable_headers.h"
#include "fmt/format.h"

namespace md {
    
template<typename T>
class jagged_vector
{
    struct dim_bounds
    {
        size_t start_idx;
        size_t end_idx;
    };
    
public:
    typedef std::vector<T> data_vector;
    typedef typename std::vector<T>::reference reference;
    typedef typename std::vector<T>::const_reference const_reference;
    typedef typename std::vector<T>::iterator data_vector_it;
    typedef typename std::vector<T>::const_iterator data_vector_cit;
    typedef typename std::vector<T>::reverse_iterator data_vector_rit;
    typedef typename std::vector<T>::const_reverse_iterator data_vector_crit;
    
    jagged_vector(size_t dim_size = 1)
    {
        size_t v_idx = 0;
        for(size_t i = 0; i < dim_size; ++i)
            _vec_dim.emplace_back((dim_bounds){v_idx, v_idx});
    }

    jagged_vector(size_t dim_size, size_t reserved_total_size)
    {
        _vec_data.reserve(reserved_total_size);
        
        size_t v_idx = 0;
        for(size_t i = 0; i < dim_size; ++i)
            _vec_dim.emplace_back((dim_bounds){v_idx, v_idx});
    }
    
    jagged_vector(const data_vector& data)
    {
        _vec_data.reserve(data.size());
        size_t v_idx = 0;
        for(size_t i = 0; i < 1; ++i)
            _vec_dim.emplace_back((dim_bounds){v_idx, data.size()});
        std::copy(data.begin(), data.end(), std::back_inserter(_vec_data));
    }
    
    jagged_vector(const std::vector<size_t> segs, const data_vector& data)
    {
        size_t v_idx = 0;
        for(size_t i = 0; i < segs.size(); ++i){
            _vec_dim.emplace_back((dim_bounds){v_idx, v_idx + segs[i]});
            v_idx += segs[i];
        }
        if(segs.size() == 0)
            return;
        
        _vec_data.reserve(data.size());
        std::copy(
            data.begin(), data.end(), std::back_inserter(_vec_data)
        );
    }
    
    jagged_vector(const std::vector<data_vector>& data)
    {
        size_t v_idx = 0;
        for(size_t i = 0; i < data.size(); ++i){
            _vec_dim.emplace_back((dim_bounds){v_idx, v_idx + data[i].size()});
            v_idx += data[i].size();
        }
        if(data.size() == 0)
            return;
        _vec_data.reserve(_vec_dim[_vec_dim.size() -1].end_idx);
        for(size_t i = 0; i < data.size(); ++i)
            std::copy(
                data[i].begin(), data[i].end(), std::back_inserter(_vec_data)
            );
    }
    
    jagged_vector(const jagged_vector& other)
        : _vec_dim(other._vec_dim),
        _vec_data(other._vec_data)
    {
    }
    jagged_vector(jagged_vector&& other)
        : _vec_dim(std::move(other._vec_dim)),
        _vec_data(std::move(other._vec_data))
    {
    }
    jagged_vector& operator=(const jagged_vector& other)
    {
        this->_vec_dim = other._vec_dim;
        this->_vec_data = other._vec_data;
        return *this;
    }
    jagged_vector& operator=(jagged_vector&& other)
    {
        this->_vec_dim = std::move(other._vec_dim);
        this->_vec_data = std::move(other._vec_data);
        return *this;
    }
    
    template<
        typename IT,
        typename ITRCV = typename std::remove_cv<T>::type
    >
    class fwd_iterator
        : public std::iterator<
            std::forward_iterator_tag,
            ITRCV,
            std::ptrdiff_t,
            IT*,
            IT&
        >
    {
        friend class jagged_vector;
        
    protected:
        explicit fwd_iterator(
            const jagged_vector<T>* v,
            size_t d,
            jagged_vector<T>::dim_bounds b,
            size_t i)
            : _v(v), _d(d), _b(b), _i(i)
        {
        }
        
    public:
        void swap(fwd_iterator& other) noexcept
        {
            std::swap(_v, other._v);
            std::swap(_d, other._d);
            std::swap(_b, other._b);
            std::swap(_i, other._i);
        }
        
        template<typename ITT = IT>
        typename std::enable_if<
            std::is_const<ITT>::value,
            const_reference
        >::type operator *() const
        {
            return (*_v)(_d, _i);
        }
        
        template<typename ITT = IT>
        typename std::enable_if<
            !std::is_const<ITT>::value,
            reference
        >::type operator *()
        {
            return (*_v)(_d, _i);
        }
        
        template<typename ITT = IT>
        typename std::enable_if<
            std::is_const<ITT>::value,
            const_reference
        >::type operator ->() const
        {
            return (*_v)(_d, _i);
        }
        
        template<typename ITT = IT>
        typename std::enable_if<
            !std::is_const<ITT>::value,
            reference
        >::type operator ->()
        {
            return (*_v)(_d, _i);
        }
        
        fwd_iterator operator +(size_t inc)
        {
            fwd_iterator copy(*this);
            copy._i += inc;
            if(copy._i >= copy._b.end_idx +1){
                throw MD_ERR("Out of bounds iterator increment!");
            }
            return copy;
        }
        
        const fwd_iterator &operator ++() // pre-increment
        {
            if(_i == _b.end_idx +1)
                throw MD_ERR("Out of bounds iterator increment!");
            ++_i;
            return *this;
        }
        
        fwd_iterator operator ++(int) // post-increment
        {
            if(_i == _b.end_idx +1)
                throw MD_ERR("Out of bounds iterator increment!");
            
            fwd_iterator copy(*this);
            ++_i;
            return copy;
        }
        
        template<typename OtherIteratorType>
        bool operator ==(const fwd_iterator<OtherIteratorType>& rhs) const
        {
            return _d == rhs._d && _i == rhs._i;
            //return _v->(_d, _i) == rhs._v->(_d, _i);
        }
        template<typename OtherIteratorType>
        bool operator !=(const fwd_iterator<OtherIteratorType>& rhs) const
        {
            return _d != rhs._d || _i != rhs._i;
            //return _v->(_d, _i) != rhs._v->(_d, _i);
        }
        
        // convert iterator to const_iterator
        operator fwd_iterator<const IT>() const
        {
            return fwd_iterator<const IT>(_v, _d, _b, _i);
        }
    
    private:
        const jagged_vector<T>* _v;
        size_t _d;
        jagged_vector<T>::dim_bounds _b;
        size_t _i;
    };
    
    typedef fwd_iterator<T> iterator;
    typedef fwd_iterator<const T> const_iterator;
    
    /**
     * Returns the number of dimensions.
     */
    size_t dim_size() const { return _vec_dim.size();}
    size_t size(size_t dim_idx) const
    {
        if(dim_idx >= dim_size())
            return 0;
        
        auto& b = _vec_dim[dim_idx];
        return b.end_idx - b.start_idx;
    }
    
    T* data(size_t dim_idx) noexcept
    {
        if(dim_idx >= dim_size())
            return nullptr;
        
        return _vec_data.data() + _vec_dim[dim_idx].start_idx;
    }
    const T* data(size_t dim_idx) const noexcept
    {
        if(dim_idx >= dim_size())
            return nullptr;
        
        return _vec_data.data() + _vec_dim[dim_idx].start_idx;
    }

    void emplace_back(size_t dim_idx)
    {
        throw std::runtime_error("no value provided!");
    }
    
    template<typename... Args>
    void emplace_back(size_t dim_idx, Args&&... v)
    {
        if(dim_idx == _vec_dim.size() -1){
            _vec_dim[_vec_dim.size()-1].end_idx += sizeof...(Args);
            _vec_data.emplace_back(v...);
            return;
        }
        
        size_t pos = 0;
        dim_bounds b{0,0};
        if(dim_idx < _vec_dim.size()){
            pos = _vec_dim[dim_idx].end_idx +1;
        }
        this->emplace(
            //dim_idx,
            const_iterator(this, dim_idx, b, pos),
            v...
        );
    }
    
    template<typename... Args>
    void emplace(const_iterator pos, Args&&... v)
    {
        // add empty dimensions
        if(pos._d >= _vec_dim.size()){
            size_t v_idx =
                _vec_dim.size() == 0 ? 0 :
                _vec_dim[_vec_dim.size()-1].end_idx;
            for(size_t i = _vec_dim.size(); i < pos._d; ++i)
                _vec_dim.emplace_back((dim_bounds){v_idx, v_idx});
        }
        
        size_t abs_i = abs_pos(pos._d, pos._i);
        size_t s = sizeof...(v);
        _vec_dim[pos._d].end_idx += s;
        for(size_t i = pos._d +1; i < _vec_dim.size(); ++i){
            _vec_dim[i].start_idx += s;
            _vec_dim[i].end_idx += s;
        }
        
        typename data_vector::iterator abs_it = _vec_data.begin() + abs_i;
        _vec_data.emplace(abs_it, v...);
    }
    
    iterator erase(
        const_iterator __first, const_iterator __last)
    {
        if(__first._d != __last._d)
            throw MD_ERR(
                "Iterators must be in same dimension"
            );
        if(__first._i >= __last._i)
            return __first;
        
        size_t abs_i = abs_pos(__first._d, __first._i);
        size_t s = __last._i - __first._i;
        _vec_dim[__first._d].end_idx -= s;
        for(size_t i = __first._d +1; i < _vec_dim.size(); ++i){
            _vec_dim[i].start_idx -= s;
            _vec_dim[i].end_idx -= s;
        }
        _vec_data.erase(abs_i, abs_i +s);
        return iterator(this, __first._d, _vec_dim[__first._d], __first._i);
    }
    iterator erase(
        const_iterator __position)
    {
        return erase(__position, __position +1);
    }
    
    iterator begin(size_t dim_idx) noexcept
    {
        dim_bounds db{0,0};
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return iterator(this, dim_idx, db, db.start_idx);
    }
    iterator end(size_t dim_idx) noexcept
    {
        dim_bounds db{0,0};
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return iterator(this, dim_idx, db, db.end_idx);
    }
    
    const_iterator cbegin(size_t dim_idx) const
    {
        dim_bounds db{0,0};
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return const_iterator(this, dim_idx, db, db.start_idx);
    }
    const_iterator cend(size_t dim_idx) const
    {
        dim_bounds db{0,0};
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return const_iterator(this, dim_idx, db, db.end_idx);
    }
    
    /**
     * add dimensions to the jagged vector
     */
    void expand(size_t dim_size) noexcept
    {
        size_t v_idx =
            _vec_dim.size() == 0 ? 0 :
            _vec_dim[_vec_dim.size()-1].end_idx;
        for(size_t i = 0; i < dim_size; ++i)
            _vec_dim.emplace_back((dim_bounds){v_idx, v_idx});
    }
    
    void clear() noexcept
    {
        for(auto& b : _vec_dim)
            b.start_idx = b.end_idx = 0;
        _vec_data.clear();
    }
    void clear(size_t dim_idx) noexcept
    {
        this->erase(
            begin(dim_idx),
            begin(dim_idx) + size(dim_idx)
        );
    }
    
    reference operator()(size_t dim_idx, size_t pos_idx)
    {
        return _vec_data.at(abs_pos(dim_idx, pos_idx));
    }
    const_reference operator()(size_t dim_idx, size_t pos_idx) const
    {
        return _vec_data.at(abs_pos(dim_idx, pos_idx));
    }
    reference operator[](size_t abs_idx)
    {
        return _vec_data[abs_idx];
    }
    const_reference operator[](size_t abs_idx) const
    {
        return _vec_data[abs_idx];
    }
    
    size_t size() const noexcept { return _vec_data.size();}
    T* data() noexcept { return _vec_data.data();}
    const T* data() const noexcept { return _vec_data.data();}
    data_vector_it begin() const noexcept
    {
        return _vec_data.begin();
    }
    data_vector_it end() const noexcept
    {
        return _vec_data.end();
    }
    data_vector_cit cbegin() const noexcept
    {
        return _vec_data.cbegin();
    }
    data_vector_cit cend() const noexcept
    {
        return _vec_data.cend();
    }
    data_vector_rit rbegin() const noexcept
    {
        return _vec_data.rbegin();
    }
    data_vector_rit rend() const noexcept
    {
        return _vec_data.rend();
    }
    data_vector_crit crbegin() const noexcept
    {
        return _vec_data.crbegin();
    }
    data_vector_crit crend() const noexcept
    {
        return _vec_data.crend();
    }
    
    size_t abs_pos(size_t dim_idx, size_t v_idx) const
    {
        if(dim_idx >= _vec_dim.size())
            return npos;
        
        if(v_idx > _vec_dim[dim_idx].end_idx)
            return npos;
        
        return _vec_dim[dim_idx].start_idx + v_idx;
    }
    
    // void insert(size_t dim_idx, const T& v, size_t v_idx = npos)
    // {
    //      
    // }
    
    static const size_t npos = static_cast<size_t>(-1);
private:
    std::vector<dim_bounds> _vec_dim;
    data_vector _vec_data;
};
    
}//::md
#endif //_tools_md_jagged_vector_h
