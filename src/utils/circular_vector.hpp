#include <vector>

template <class T>
class CircularVector
{
  public:
    CircularVector(size_t maxSizeIn, bool reserveIn = false) :
        maxSize(maxSizeIn), reserve(reserveIn)
    {
        clear();
    }

    template <class... Args>
    void emplace(Args&&... args)
    {
        if (maxSize == 0)
        {
            return;
        }
        if (vec.size() == maxSize)
        {
            vec.at(idx) = std::move(T(args...));
        }
        else
        {
            vec.emplace_back(args...);
        }
        idx = (idx + 1 == maxSize ? 0 : idx + 1);
    }

    void clear()
    {
        vec.clear();
        idx = 0;
        if (reserve)
        {
            vec.reserve(maxSize);
        }
    }

    bool isFull() const
    {
        return vec.size() == maxSize;
    }

    typename std::vector<T>::const_iterator begin() const
    {
        return vec.begin();
    }

    typename std::vector<T>::const_iterator end() const
    {
        return vec.end();
    }

  private:
    size_t maxSize;
    size_t idx = 0;
    bool reserve;
    std::vector<T> vec;
};
