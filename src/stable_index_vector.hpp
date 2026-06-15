#pragma once
#include <limits>
#include <vector>
#include <cassert>

namespace siv
{
    using ID = uint64_t;
    static constexpr ID InvalidID = std::numeric_limits<ID>::max();

    template<typename TObjectType>
    class Vector;

    template<typename TObjectType>
    class Handle
    {
    public:
        Handle() = default;
        Handle(ID id, ID validity_id, Vector<TObjectType>* vector)
            : m_id{id}
            , m_validity_id{validity_id}
            , m_vector{vector}
        {}

        TObjectType* operator->() { return &(*m_vector)[m_id]; }
        TObjectType const* operator->() const { return &(*m_vector)[m_id]; }
        TObjectType& operator*() { return (*m_vector)[m_id]; }
        TObjectType const& operator*() const { return (*m_vector)[m_id]; }

        [[nodiscard]] ID getID() const { return m_id; }
        explicit operator bool() const { return isValid(); }
        [[nodiscard]] bool isValid() const { return m_vector && m_vector->isValid(m_id, m_validity_id); }

    private:
        ID m_id = 0;
        ID m_validity_id = 0;
        Vector<TObjectType>* m_vector = nullptr;
        friend class Vector<TObjectType>;
    };

    template<typename TObjectType>
    class Vector
    {
    public:
        Vector() = default;

        ID push_back(const TObjectType& object) {
            const ID id = getFreeSlot();
            m_data.push_back(object);
            return id;
        }

        template<typename... TArgs>
        ID emplace_back(TArgs&&... args) {
            const ID id = getFreeSlot();
            m_data.emplace_back(std::forward<TArgs>(args)...);
            return id;
        }

        void erase(ID id) {
            const ID data_id = m_indexes[id];
            const ID last_data_id = m_data.size() - 1;
            const ID last_id = m_metadata[last_data_id].rid;
            ++m_metadata[data_id].validity_id;
            std::swap(m_data[data_id], m_data[last_data_id]);
            std::swap(m_metadata[data_id], m_metadata[last_data_id]);
            std::swap(m_indexes[id], m_indexes[last_id]);
            m_data.pop_back();
        }

        void erase(const Handle<TObjectType>& handle) {
            assert(handle.m_vector == this);
            assert(handle.isValid());
            erase(handle.getID());
        }

        TObjectType& operator[](ID id) { return m_data[m_indexes[id]]; }
        TObjectType const& operator[](ID id) const { return m_data[m_indexes[id]]; }

        size_t size() const { return m_data.size(); }
        bool empty() const { return m_data.empty(); }

        Handle<TObjectType> createHandle(ID id) {
            assert(getDataIndex(id) < size());
            return {id, m_metadata[m_indexes[id]].validity_id, this};
        }

        bool isValid(ID id, ID validity_id) const {
            return validity_id == m_metadata[m_indexes[id]].validity_id;
        }

        // Expose data access for ladder to use
        TObjectType* get(ID id) { 
            if (id < m_indexes.size() && m_indexes[id] < m_data.size()) return &m_data[m_indexes[id]];
            return nullptr;
        }
        
        // Add const overload for get
        const TObjectType* get(ID id) const { 
            if (id < m_indexes.size() && m_indexes[id] < m_data.size()) return &m_data[m_indexes[id]];
            return nullptr;
        }

    private:
        ID getFreeSlot() {
            const ID id = getFreeID();
            if (id >= m_indexes.size()) m_indexes.resize(id + 1);
            m_indexes[id] = m_data.size();
            return id;
        }

        ID getFreeID() {
            if (m_metadata.size() > m_data.size()) {
                ++m_metadata[m_data.size()].validity_id;
                return m_metadata[m_data.size()].rid;
            }
            const ID new_id = m_data.size();
            m_metadata.push_back({new_id, 0});
            m_indexes.push_back(new_id);
            return new_id;
        }

        struct Metadata {
            ID rid = 0;
            ID validity_id = 0;
        };

        std::vector<TObjectType> m_data;
        std::vector<Metadata> m_metadata;
        std::vector<ID> m_indexes;
        uint64_t getDataIndex(ID id) const { return m_indexes[id]; }
    };
}
