/// test_exceptions.cpp — exception hierarchy unit tests

#include <gtest/gtest.h>
#include <stdexcept>
#include "deathbycaptcha/deathbycaptcha.hpp"

TEST(AccessDeniedException, IsRuntimeError) {
    try {
        throw dbc::AccessDeniedException("bad creds");
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "bad creds");
    }
}

TEST(AccessDeniedException, IsException) {
    try {
        throw dbc::AccessDeniedException("x");
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "x");
    }
}

TEST(ServiceOverloadException, IsRuntimeError) {
    try {
        throw dbc::ServiceOverloadException("overloaded");
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "overloaded");
    }
}

TEST(ServiceOverloadException, IsException) {
    try {
        throw dbc::ServiceOverloadException("y");
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "y");
    }
}

TEST(ExceptionHierarchy, DistinctTypes) {
    // AccessDeniedException is not a ServiceOverloadException
    bool caught_wrong = false;
    try {
        throw dbc::AccessDeniedException("z");
    } catch (const dbc::ServiceOverloadException&) {
        caught_wrong = true;
    } catch (const std::exception&) {
        // correct
    }
    EXPECT_FALSE(caught_wrong);
}
