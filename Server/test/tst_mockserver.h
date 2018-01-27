#ifndef TST_MOCKSERVER_H
#define TST_MOCKSERVER_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/server.h"

using namespace testing;


class MockServer : public Server {
 public:
  MOCK_METHOD0(run,int());
  MOCK_METHOD1(onClientBlocked,
      void(std::unique_ptr<ClientServerData>& l_client));
  MOCK_METHOD1(onClientRejected,
      void(std::unique_ptr<ClientServerData>& l_client));
  MOCK_METHOD1(onClientConnected,
      void(std::unique_ptr<ClientServerData>&));
  MOCK_METHOD1(onClientDisconnected,
      void(std::unique_ptr<ClientServerData>& l_client));
  MOCK_METHOD2(onClientMessageReceived,
      void(std::unique_ptr<ClientServerData>& l_client, const std::string& l_text));
  MOCK_METHOD2(onClientPromoted,
      void(std::unique_ptr<ClientServerData>& l_client, const bool& l_promoted));
  MOCK_METHOD1(onErrorWithReceivingData,
      void(std::unique_ptr<ClientServerData>& l_client));
  MOCK_METHOD1(onErrorWithSendingData,
      void(std::unique_ptr<ClientServerData>& l_client));
  MOCK_METHOD1(onArgumentsError,
      void(const char*));
  MOCK_METHOD1(error,
      void(const std::string& l_error));
};




#endif // TST_MOCKSERVER_H
