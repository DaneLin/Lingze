#pragma once

#include <vector>

namespace Utils
{
    template<typename T>
    struct Pool
    {
        struct Id
        {
            Id(size_t val) : as_int(val) {}
            Id() : as_int(size_t(-1)) {}
            bool operator == (const Id& other) const { return this->as_int == other.as_int; }
            size_t as_int;
        };
        struct Iterator
        {
            Iterator(Id id, Pool<T>& pool) :
                id_(id),
                pool_(pool)
            {
                skip();
            }
            T& operator *()
            {
                return pool_.get(id_);
            }
            void operator++()
            {
                ++id_.as_int;
                skip();
            }
            bool operator != (const Iterator& other)
            {
                return this->id_.as_int != other.id_.as_int;
            }
        private:
            void skip()
            {
                for (; id_.as_int < pool_.get_size() && !pool_.is_present(id_); ++id_.as_int);
            }
            Id id_;
            Pool<T>& pool_;
        };

        Iterator begin()
        {
            return Iterator({ 0 }, *this);
        }
        Iterator end()
        {
            return Iterator({ get_size() }, *this);
        }

        Id add(T&& elem)
        {
            if (free_ids_.size() > 0)
            {
                Id id = { free_ids_.back() };
                free_ids_.pop_back();
                data_[id.as_int] = elem;
                is_present_[id.as_int] = true;
                return id;
            }
            else
            {
                Id id = { data_.size() };
                data_.push_back(elem);
                is_present_.push_back(true);
                return id;
            }
        }
        void release(const Id& id)
        {
            assert(is_present_[id.as_int]);
            is_present_[id.as_int] = false;
            free_ids_.push_back(id);
        }
        T& get(const Id& id)
        {
            assert(id.as_int != size_t(-1));
            assert(id.as_int < data_.size());
            assert(is_present_[id.as_int]);
            return data_[id.as_int];
        }
        size_t get_size()
        {
            return data_.size();
        }
        bool is_present(Id id)
        {
            return is_present_[id.as_int];
        }
    private:
        std::vector<T> data_;
        std::vector<bool> is_present_;
        std::vector<Id> free_ids_;
    };
}