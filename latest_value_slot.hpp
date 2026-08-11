#ifndef LATEST_VALUE_SLOT_HPP
#define LATEST_VALUE_SLOT_HPP

#include <mutex>
#include <utility>

template <typename T>
class LatestValueSlot
{
public:
    void publish(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = value;
        m_hasValue = true;
    }

    void publish(T&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = std::move(value);
        m_hasValue = true;
    }

    bool takeLatest(T& output)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_hasValue)
        {
            return false;
        }
        output = m_value;
        m_hasValue = false;
        return true;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = T();
        m_hasValue = false;
    }

private:
    std::mutex m_mutex;
    T m_value{};
    bool m_hasValue = false;
};

#endif  // LATEST_VALUE_SLOT_HPP
