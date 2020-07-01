#pragma once

namespace utils
{

struct CompareUnderlyingObjects
{
    template <template <typename> class PointerTo, class T>
    bool operator()(const PointerTo<T>& left, const PointerTo<T>& right)
    {
        bool isLeft = left;
        bool isRight = right;

        if (isLeft && isRight)
        {
            return *left == *right;
        }

        return isLeft == isRight;
    }
};

struct CompareLessUnderlyingObjects
{
    template <template <typename> class PointerTo, class T>
    bool operator()(const PointerTo<T>& left, const PointerTo<T>& right)
    {
        bool isLeft = left;
        bool isRight = right;

        if (isLeft && isRight)
        {
            return *left < *right;
        }

        return isLeft < isRight;
    }
};

} // namespace utils
