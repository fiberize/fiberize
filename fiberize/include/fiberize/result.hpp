/**
 * Container that holds other a value or an exception.
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
template <typename Value>
class Result
{
public:
    /**
     * @name Constructors and destructors
     */
    ///@{

    /**
     * Copies a value.
     */
    Result(const Value& value) {
        construct(value);
    }

    /**
     * Moves a value.
     */
    Result(Value&& value) {
        construct(std::move(value));
    }

    /**
     * Constructs an exception.
     */
    Result(std::exception_ptr exception) {
        construct(exception);
    }

    /**
     * Copies an existing result.
     */
    Result(const Result& other) {
        construct(other);
    }

    /**
     * Moves an existing result.
     */
    Result(Result&& other) {
        construct(other);
    }

    /**
     * Destroys the result.
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
     * Assigns an existing result.
     */
    Result& operator = (const Result& other) {
        destruct();
        construct(other);
        return *this;
    }

    /**
     * Moves an existing result.
     */
    Result& operator = (Result&& other) {
        destruct();
        construct(std::move(other));
        return *this;
    }

    /**
     * Assigns an existing value.
     */
    Result& operator = (const Value& value) {
        destruct();
        construct(value);
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
    Value& get() {
        if (isValue()) {
            return value_;
        } else {
            std::rethrow_exception(exception_);
        }
    }

    /**
     * Returns the contained value or raises the exception.
     */
    const Value& get() const {
        if (isValue()) {
            return value_;
        } else {
            std::rethrow_exception(exception_);
        }
    }

    ///@}

private:
    void construct(const Value& value) {
        new (&value_) Value(value);
        isValue_ = true;
    }

    void construct(Value&& value) {
        new (&value_) Value(std::move(value));
        isValue_ = true;
    }

    void construct(std::exception_ptr excception) {
        new (&exception_) std::exception_ptr(std::move(excception));
        isValue_ = false;
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
            value_.~Value();
        } else {
            exception_.~exception_ptr();
        }
    }

    bool isValue_;
    union {
        Value value_;
        std::exception_ptr exception_;
    };
};

/**
 * Container that holds either nothing or an exception.
 *
 * @ingroup fiberize
 */
template <>
class Result<void>
{
public:
    /**
     * @name Constructors and destructors
     */
    ///@{

    /**
     * Constructs an empty result, with no exceptions.
     */
    Result() {
        construct();
    }

    /**
     * Constructs an exception.
     */
    Result(std::exception_ptr exception) {
        construct(exception);
    }

    /**
     * Copies an existing result.
     */
    Result(const Result& other) {
        construct(other);
    }

    /**
     * Moves an existing result.
     */
    Result(Result&& other) {
        construct(other);
    }

    /**
     * Destroys the result.
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
     * Assigns an existing result.
     */
    Result& operator = (const Result& other) {
        destruct();
        construct(other);
        return *this;
    }

    /**
     * Moves an existing result.
     */
    Result& operator = (Result&& other) {
        destruct();
        construct(std::move(other));
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
     * Does nothing or raises the exception.
     */
    void get() const {
        if (isException()) {
            std::rethrow_exception(exception_);
        }
    }

    ///@}

private:
    void construct() {
        isValue_ = true;
    }

    void construct(std::exception_ptr excception) {
        new (&exception_) std::exception_ptr(std::move(excception));
        isValue_ = false;
    }

    void construct(const Result& other) {
        if (other.isValue()) {
            construct();
        } else {
            construct(other.exception_);
        }
    }

    void construct(Result&& other) {
        if (other.isValue()) {
            construct();
        } else {
            construct(std::move(other.exception_));
        }
    }

    void destruct() {
        if (isException()) {
            exception_.~exception_ptr();
        }
    }

    bool isValue_;
    union {
        std::exception_ptr exception_;
    };
};

} // namespace fiberize

#endif // FIBERIZE_RESULT_HPP
