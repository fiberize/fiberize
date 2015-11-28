/**
 * Container that holds either a value or an exception.
 *
 * @file result.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_RESULT_HPP
#define FIBERIZE_RESULT_HPP

#include <exception>

namespace fiberize {

/**
 * Container that holds either a value or an exception.
 *
 * @ingroup fiberize
 */
template <typename A>
class Result
{
public:
    /**
     * @name Constructors and destructors
     */
    ///@{

    /**
     * Constructs a value with the given arguments.
     */
    template <typename... Args>
    Result(Args&&... args) {
        construct(std::forward<Args>(args)...);
    }

    /**
     * Constructs an exception.
     */
    Result(std::exception_ptr exception) {
        construct(exception);
    }

    /**
     * Copies an existing object.
     */
    Result(const Result& other) {
        construct(other);
    }

    /**
     * Moves an existing object.
     */
    Result(Result&& other) {
        construct(other);
    }

    /**
     * Destroys the object.
     */
    ~Result() {
        destruct();
    }

    ///@}

    /**
     * @name Assignment
     */
    ///@{

    /**
     * Assigns an existing object.
     */
    Result& operator = (const Result& either) {
        destruct();
        construct(either);
        return *this;
    }

    /**
     * Moves an existing object.
     */
    Result& operator = (Result&& either) {
        destruct();
        construct(std::move(either));
        return *this;
    }

    ///@}

    /**
     * @name Properties
     */
    ///@{

    /**
     * Is this a value?
     */
    bool isValue() const {
        return isValue_;
    }

    /**
     * Is this an exception?
     */
    bool isException() const {
        return !isValue_;
    }

    /**
     * Returns the contained value or raises the exception.
     */
    A& get() {
        if (isValue()) {
            return value_;
        } else {
            std::rethrow_exception(exception_);
        }
    }

    /**
     * Returns the contained value or raises the exception.
     */
    const A& get() const {
        if (isValue()) {
            return value_;
        } else {
            std::rethrow_exception(exception_);
        }
    }

    ///@}

private:
    template <typename... Args>
    void construct(Args&&... args) {
        new (&value_) A(std::forward<Args>(args)...);
    }

    void construct(std::exception_ptr excception) {
        new (&exception_) std::exception_ptr(std::move(excception));
    }

    void construct(const Result& other) {
        if (other.isValue()) {
            construct(other.value_);
        } else {
            construct(other.exception_);
        }
        isValue_ = other.isValue_;
    }

    void construct(Result&& other) {
        if (other.isValue()) {
            construct(std::move(other.value_));
        } else {
            construct(std::move(other.exception_));
        }
        isValue_ = other.isValue_;
    }

    void destruct() {
        if (isValue_) {
            value_.~A();
        } else {
            exception_.~exception_ptr();
        }
    }

    bool isValue_;
    union {
        A value_;
        std::exception_ptr exception_;
    };
};

} // namespace fiberize

#endif // FIBERIZE_RESULT_HPP
