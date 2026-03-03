#include <gtest/gtest.h>

#include "core/net/net.hpp"

// NetError Tests
TEST(NetError, DefaultConstructorNoError) {
  NetError error;

  EXPECT_FALSE(error.hasError());
  EXPECT_EQ(error.getCode(), static_cast<int>(NetErrorType::NONE));
  EXPECT_EQ(error.getError(), NetErrorType::NONE);
  EXPECT_TRUE(error.getMessage().empty());
}

TEST(NetError, ConstructWithError) {
  NetError error(NetErrorType::RESOLVE_HOST_ERR, "Failed to resolve host");

  EXPECT_TRUE(error.hasError());
  EXPECT_EQ(error.getCode(), static_cast<int>(NetErrorType::RESOLVE_HOST_ERR));
  EXPECT_EQ(error.getError(), NetErrorType::RESOLVE_HOST_ERR);
  EXPECT_EQ(error.getMessage(), "Failed to resolve host");
}

TEST(NetError, SetError) {
  NetError error;

  error.setError(NetErrorType::SSL_HANDSHAKE_ERR, "SSL handshake failed");

  EXPECT_TRUE(error.hasError());
  EXPECT_EQ(error.getCode(), static_cast<int>(NetErrorType::SSL_HANDSHAKE_ERR));
  EXPECT_EQ(error.getError(), NetErrorType::SSL_HANDSHAKE_ERR);
  EXPECT_EQ(error.getMessage(), "SSL handshake failed");
}

TEST(NetError, ResetClearsError) {
  NetError error(NetErrorType::WS_HANDSHAKE_ERR, "WebSocket handshake failed");

  EXPECT_TRUE(error.hasError());

  error.reset();

  EXPECT_FALSE(error.hasError());
  EXPECT_EQ(error.getCode(), static_cast<int>(NetErrorType::NONE));
  EXPECT_EQ(error.getError(), NetErrorType::NONE);
  EXPECT_TRUE(error.getMessage().empty());
}

TEST(NetError, MultipleSetErrors) {
  NetError error;

  error.setError(NetErrorType::TCP_CONNECT_ERR, "First error");
  EXPECT_EQ(error.getMessage(), "First error");

  error.setError(NetErrorType::READ_BUFFER_ERR, "Second error");
  EXPECT_EQ(error.getMessage(), "Second error");
  EXPECT_EQ(error.getError(), NetErrorType::READ_BUFFER_ERR);
}

TEST(NetError, AllErrorTypes) {
  std::vector<std::pair<NetErrorType, std::string>> errorTypes = {
      {NetErrorType::UNDEFINED, "Undefined error"},
      {NetErrorType::NONE, "No error"},
      {NetErrorType::RESOLVE_HOST_ERR, "Resolve host error"},
      {NetErrorType::SSL_HANDSHAKE_ERR, "SSL handshake error"},
      {NetErrorType::WS_HANDSHAKE_ERR, "WebSocket handshake error"},
      {NetErrorType::TCP_CONNECT_ERR, "TCP connect error"},
      {NetErrorType::READ_BUFFER_ERR, "Read buffer error"},
      {NetErrorType::WRITE_BUFFER_ERR, "Write buffer error"},
      {NetErrorType::SHUTDOWN_ERR, "Shutdown error"},
      {NetErrorType::INVALID_SOCKET_ERR, "Invalid socket error"},
      {NetErrorType::INVALID_QUERY_ERR, "Invalid query error"}};

  for (const auto &[type, message] : errorTypes) {
    NetError error(type, message);

    EXPECT_EQ(error.getError(), type);
    EXPECT_EQ(error.getCode(), static_cast<int>(type));
    EXPECT_EQ(error.getMessage(), message);

    if (type == NetErrorType::NONE) {
      EXPECT_FALSE(error.hasError());
    } else {
      EXPECT_TRUE(error.hasError());
    }
  }
}

TEST(NetError, EmptyMessage) {
  NetError error(NetErrorType::TCP_CONNECT_ERR, "");

  EXPECT_TRUE(error.hasError());
  EXPECT_TRUE(error.getMessage().empty());
}

TEST(NetError, LongMessage) {
  std::string longMsg(1000, 'x');
  NetError error(NetErrorType::READ_BUFFER_ERR, longMsg);

  EXPECT_EQ(error.getMessage(), longMsg);
}

TEST(NetError, CopyConstructor) {
  NetError error1(NetErrorType::SSL_HANDSHAKE_ERR, "Original error");
  NetError error2 = error1;

  EXPECT_EQ(error2.getError(), NetErrorType::SSL_HANDSHAKE_ERR);
  EXPECT_EQ(error2.getMessage(), "Original error");
  EXPECT_TRUE(error2.hasError());
}

TEST(NetError, AssignmentOperator) {
  NetError error1(NetErrorType::WS_HANDSHAKE_ERR, "Error 1");
  NetError error2;

  error2 = error1;

  EXPECT_EQ(error2.getError(), NetErrorType::WS_HANDSHAKE_ERR);
  EXPECT_EQ(error2.getMessage(), "Error 1");
  EXPECT_TRUE(error2.hasError());
}

TEST(NetError, ConsecutiveResets) {
  NetError error(NetErrorType::TCP_CONNECT_ERR, "Error");

  error.reset();
  error.reset();
  error.reset();

  EXPECT_FALSE(error.hasError());
}

TEST(NetError, SetAfterReset) {
  NetError error(NetErrorType::SSL_HANDSHAKE_ERR, "First");

  error.reset();
  EXPECT_FALSE(error.hasError());

  error.setError(NetErrorType::READ_BUFFER_ERR, "Second");
  EXPECT_TRUE(error.hasError());
  EXPECT_EQ(error.getMessage(), "Second");
}

TEST(NetError, UndefinedErrorType) {
  NetError error(NetErrorType::UNDEFINED, "Undefined error occurred");

  EXPECT_TRUE(error.hasError());
  EXPECT_EQ(error.getError(), NetErrorType::UNDEFINED);
  EXPECT_EQ(error.getCode(), static_cast<int>(NetErrorType::UNDEFINED));
}

TEST(NetError, NoneErrorTypeDoesNotHaveError) {
  NetError error(NetErrorType::NONE, "This should not be an error");

  EXPECT_FALSE(error.hasError());
  EXPECT_EQ(error.getError(), NetErrorType::NONE);
}

// NetworkException Tests
TEST(NetworkException, DefaultConstruction) {
  NetworkException ex("Test exception");

  EXPECT_STREQ(ex.what(), "Test exception");
}

TEST(NetworkException, ConstructionWithErrorType) {
  NetworkException ex("WebSocket error", NetErrorType::WS_HANDSHAKE_ERR);

  EXPECT_STREQ(ex.what(), "WebSocket error");
}

TEST(NetworkException, UndefinedErrorType) {
  NetworkException ex("Undefined error");

  EXPECT_STREQ(ex.what(), "Undefined error");
}

TEST(NetworkException, EmptyMessage) {
  NetworkException ex("");

  EXPECT_STREQ(ex.what(), "");
}

TEST(NetworkException, LongMessage) {
  std::string longMsg(1000, 'e');
  NetworkException ex(longMsg);

  EXPECT_EQ(std::string(ex.what()), longMsg);
}

TEST(NetworkException, ThrowAndCatch) {
  try {
    throw NetworkException("Test throw", NetErrorType::TCP_CONNECT_ERR);
  } catch (const NetworkException &ex) {
    EXPECT_STREQ(ex.what(), "Test throw");
  } catch (...) {
    FAIL() << "Expected NetworkException";
  }
}

TEST(NetworkException, CatchAsStdException) {
  try {
    throw NetworkException("Standard exception test");
  } catch (const std::exception &ex) {
    EXPECT_STREQ(ex.what(), "Standard exception test");
  } catch (...) {
    FAIL() << "Expected std::exception";
  }
}

TEST(NetworkException, MultipleExceptionTypes) {
  std::vector<NetErrorType> types = {
      NetErrorType::RESOLVE_HOST_ERR, NetErrorType::SSL_HANDSHAKE_ERR,
      NetErrorType::WS_HANDSHAKE_ERR, NetErrorType::TCP_CONNECT_ERR};

  for (const auto &type : types) {
    try {
      throw NetworkException("Error occurred", type);
    } catch (const NetworkException &ex) {
      EXPECT_STREQ(ex.what(), "Error occurred");
    }
  }
}

// Integration Tests
TEST(NetErrorIntegration, ErrorWorkflow) {
  NetError error;

  // Initial state
  EXPECT_FALSE(error.hasError());

  // Simulate connection error
  error.setError(NetErrorType::TCP_CONNECT_ERR, "Connection refused");
  EXPECT_TRUE(error.hasError());

  // Retry and reset
  error.reset();
  EXPECT_FALSE(error.hasError());

  // New error
  error.setError(NetErrorType::SSL_HANDSHAKE_ERR,
                 "Certificate validation failed");
  EXPECT_TRUE(error.hasError());
  EXPECT_EQ(error.getError(), NetErrorType::SSL_HANDSHAKE_ERR);
}

TEST(NetErrorIntegration, ErrorChaining) {
  std::vector<NetError> errors;

  errors.push_back(
      NetError(NetErrorType::RESOLVE_HOST_ERR, "DNS lookup failed"));
  errors.push_back(
      NetError(NetErrorType::TCP_CONNECT_ERR, "Connection timeout"));
  errors.push_back(NetError(NetErrorType::SSL_HANDSHAKE_ERR, "SSL error"));

  int errorCount = 0;
  for (const auto &error : errors) {
    if (error.hasError()) {
      errorCount++;
    }
  }

  EXPECT_EQ(errorCount, 3);
}

TEST(NetErrorIntegration, ConditionalErrorHandling) {
  auto processRequest = [](bool shouldFail) -> NetError {
    NetError error;
    if (shouldFail) {
      error.setError(NetErrorType::WRITE_BUFFER_ERR, "Failed to write");
    }
    return error;
  };

  NetError success = processRequest(false);
  EXPECT_FALSE(success.hasError());

  NetError failure = processRequest(true);
  EXPECT_TRUE(failure.hasError());
  EXPECT_EQ(failure.getError(), NetErrorType::WRITE_BUFFER_ERR);
}
