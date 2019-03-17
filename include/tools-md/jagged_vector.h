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

    jagged_vector(size_t dim)
        :_dim(dim)
    {
    }
    
    size_t dim() const { return _vec_dim.size();}
    size_t size(size_t dim_idx) const
    {
        if(dim_idx >= dim())
            return 0;
        
        auto& b = _vec_dim[dim_idx];
        return b.end_idx - b.start_idx;
    }
    
    T* data(size_t dim_idx) noexcept
    {
        if(dim_idx >= dim())
            return nullptr;
        
        return _vec_data.data() + _vec_dim[dim_idx].start_idx;
    }
    const T* data(size_t dim_idx) const noexcept
    {
        if(dim_idx >= dim())
            return nullptr;
        
        return _vec_data.data() + _vec_dim[dim_idx].start_idx;
    }
    
    void emplace_back(size_t dim_idx, const T& v)
    {
        
        //this->emplace_back(v);
    }
    
    void emplace(size_t dim_idx, , const T& v)
    {
        
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
        explicit fwd_iterator(const jagged_vector<T>* jv,
            jagged_vector<T>::dim_bounds r, size_t idx)
            //ITRCV v)
            : _jv(jv), _r(r), _i(idx)
        {
        }
        
    public:
        void swap(fwd_iterator& other) noexcept
        {
            std::swap(_jv, other._jv);
            std::swap(_r, other._r);
            std::swap(_i, other._i);
        }
        
        IT& operator *() const
        {
            return _jv[_i];
        }
        
        IT& operator *()
        {
            return _jv[_i];
        }
        
        IT& operator ->() const
        {
            return _jv[_i];
        }
        
        IT& operator ->()
        {
            return _jv[_i];
        }
        
        const fwd_iterator &operator ++() // pre-increment
        {
            if(_i == _r.end_idx +1)
                throw pq_async::exception("Out of bounds iterator increment!");
            ++_i;
            return *this;
        }
        
        fwd_iterator operator ++(int) // post-increment
        {
            if(_i == _r.end_idx +1)
                throw pq_async::exception("Out of bounds iterator increment!");
            
            fwd_iterator copy(*this);
            ++_i;
            return copy;
        }
        
        template<typename OtherIteratorType>
        bool operator ==(const fwd_iterator<OtherIteratorType>& rhs) const
        {
            return _jv[_i] == rhs._jv[_i];
        }
        template<typename OtherIteratorType>
        bool operator !=(const fwd_iterator<OtherIteratorType>& rhs) const
        {
            return _jv[_i] != rhs._jv[_i];
        }
        
        // convert iterator to const_iterator
        operator fwd_iterator<const IT>() const
        {
            return fwd_iterator<const IT>(_jv, _r, _i);
        }
    
    private:
        const jagged_vector<T>* _jv;
        jagged_vector<T>::dim_bounds _r;
        size_t _i;
    };
    
    typedef fwd_iterator<T> iterator;
    typedef fwd_iterator<const T> const_iterator;
    
    iterator begin(size_t dim_idx) const
    {
        dim_bounds db;
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return iterator(this, db, db.start_idx);
    }
    iterator end(size_t dim_idx) const
    {
        dim_bounds db;
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return iterator(this, db, db.end_idx);
    }

    const_iterator cbegin(size_t dim_idx) const
    {
        dim_bounds db;
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return const_iterator(this, db, db.start_idx);
    }
    const_iterator cend(size_t dim_idx) const
    {
        dim_bounds db;
        if(dim_idx < _vec_dim.size())
            db = _vec_dim[dim_idx];
        return const_iterator(this, db, db.end_idx);
    }
    
    
    
    size_t size() const noexcept { return _vec_data.size();}
    T* data() noexcept { return _vec_data.data();}
    const T* data() const noexcept { return _vec_data.data();}
    std::vector<T>::iterator begin() const noexcept
    {
        return _vec_data.begin();
    }
    std::vector<T>::iterator end() const noexcept
    {
        return _vec_data.end();
    }
    std::vector<T>::const_iterator cbegin() const noexcept
    {
        return _vec_data.cbegin();
    }
    std::vector<T>::const_iterator cend() const noexcept
    {
        return _vec_data.cend();
    }
    std::vector<T>::reverse_iterator rbegin() const noexcept
    {
        return _vec_data.rbegin();
    }
    std::vector<T>::reverse_iterator rend() const noexcept
    {
        return _vec_data.rend();
    }
    std::vector<T>::const_iterator crbegin() const noexcept
    {
        return _vec_data.crbegin();
    }
    std::vector<T>::const_iterator crend() const noexcept
    {
        return _vec_data.crend();
    }
    
    size_t abs_pos(size_t dim_idx, size_t v_idx)
    {
        if(dim_idx >= _vec_dim.size())
            return npos;
        
        if(v_idx > _vec_dim[dim_idx].end_idx)
            return npos;
        
        return _vec_dim[dim_idx].start_idx + v_idx;
    }
    
    void insert(size_t dim_idx, const T& v, size_t v_idx = npos)
    {
        
    }
    
    static const size_t npos = static_cast<size_t>(-1);
private:

    size_t _dim;
    std::vector<dim_bounds> _vec_dim;
    std::vector<T> _vec_data;
};
    
}//::md
#endif //_tools_md_jagged_vector_h
