#ifndef NETWORK_MONITOR_STOMP_CLIENT_H
#define NETWORK_MONITOR_STOMP_CLIENT_H

#include <network-monitor/StompFrame.h>
#include <network-monitor/WebSocketClient.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <string>
#include <string_view>
#include <unordered_map>

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;

namespace NetworkMonitor
{

    /*! \brief Error codes for the STOMP client.
     */
    enum class StompClientError
    {
        kOk = 0,
        kUndefinedError,
        kCouldNotCloseWebSocketConnection,
        kCouldNotConnectToWebSocketServer,
        kCouldNotParseMessageAsStompFrame,
        kCouldNotSendStompFrame,
        kCouldNotSendMessage,
        kCouldNotSendSubscribeFrame,
        kUnexpectedCouldNotCreateValidFrame,
        kUnexpectedMessageContentType,
        kUnexpectedSubscriptionMismatch,
        kWebSocketServerDisconnected,
    };

    /*! \brief Print operator for the StompClientError class.
    */
    std::ostream& operator<<(std::ostream& os, const StompClientError& m);

    /*! \brief Convert StompClientError to string.
    */
    std::string ToString(const StompClientError& m);

    /*! \brief STOMP client implementing the subset of errors needed by the
     *         network-events service.
     *
     *  \tparam WsClient    WebSocket client class. This type must have the same
     *                      interface of WebSocketClient.
     */
    template <typename WsClient>
    class StompClient
    {
    public:
        /*! \brief Construct a STOMP client connecting to a remote URL/port through
         *         a secure WebSocket connection.
         *
         *  \note This constructor does not initiate a connection.
         *
         *  \param url      The URL of the server.
         *  \param endpoint The endpoint on the server to connect to.
         *                  Example: ltnm.learncppthroughprojects.com/<endpoint>
         *  \param port     The port on the server.
         *  \param ioc      The io_context object. The user takes care of calling
         *                  ioc.run().
         *  \param ctx      The TLS context to setup a TLS socket stream.
         */
        StompClient(
            const std::string &url,
            const std::string &endpoint,
            const std::string &port,
            boost::asio::io_context &ioc,
            boost::asio::ssl::context &ctx) : m_url{url}, wsClient{url, endpoint, port, ioc, ctx}, m_context{boost::asio::make_strand(ioc)}
        {
            spdlog::info("StompClient: Creating STOMP client for {}:{}{}",
                     url, port, endpoint);
        }

        /*! \brief The copy constructor is deleted.
         */
        StompClient(const StompClient &other) = delete;

        /*! \brief Move constructor.
         */
        StompClient(StompClient &&other) = default;

        /*! \brief The copy assignment operator is deleted.
         */
        StompClient &operator=(const StompClient &other) = delete;

        /*! \brief Move assignment operator.
         */
        StompClient &operator=(StompClient &&other) = default;

        /*! \brief Connect to the STOMP server.
         */
        void Connect(std::string username, std::string password,
                     std::function<void(StompClientError)> onConnect = nullptr,
                     std::function<
                        void (StompClientError, const std::string&, std::string&&)
                     > onMessage = nullptr,
                     std::function<void(StompClientError)> onDisconnect = nullptr)
        {
            spdlog::info("StompClient: Connecting to STOMP server {}", m_url);
            m_onConnect = onConnect;
            m_onMessage = onMessage;
            m_onDisconnect = onDisconnect;
            m_username = username;
            m_password = password;

            wsClient.Connect([this](auto ec)
                             { onWebSocketConnect(ec); },
                             [this](auto ec, auto msg)
                             { onWebSocketReceive(ec, std::move(msg)); },
                             [this](auto ec)
                             { onWebSocketDisconnect(ec); });
        }

        /*! \brief Close the STOMP and WebSocket connection.
         */
        void Close(
            std::function<void(StompClientError)> onClose = nullptr)
        {
            spdlog::info("StompClient: Closing connection to STOMP server");
            m_subscriptions.clear();
            wsClient.Close(
                [this, onClose](auto ec)
                {
                    OnWsClose(ec, onClose);
                });
        }

        /*! \brief Subscribe to a STOMP endpoint.
         *
         *  \returns The subscription ID.
         */
        std::string Subscribe(
            const std::string &destination,
            std::function<void(StompClientError, std::string &&)> onSubscribe,
            std::function<void(StompClientError, std::string &&)> onMessage)
        {
            spdlog::info("StompClient: Subscribing to {}", destination);
            auto subscriptionId{GenerateId()};
            Subscription subscription{
                destination,
                onSubscribe,
                onMessage,
            };
            StompError frameError;
            StompFrame subscribeFrame = {frameError,
                                         StompCommand::kSubscribe,
                                         {
                                             {StompHeader::kId, subscriptionId},
                                             {StompHeader::kDestination, destination},
                                             {StompHeader::kAck, "auto"},
                                             {StompHeader::kReceipt, subscriptionId},
                                         }};

            if (frameError != StompError::kOk)
            {
                spdlog::error("StompClient: Could not create a valid frame: {}",
                              frameError);
                return "";
            }

            wsClient.Send(subscribeFrame.ToString(),
                          [this,
                           subscriptionId,
                           subscription = std::move(subscription)](auto ec) mutable
                          { onWebSocketSendSubscribtionFrame(
                                ec,
                                std::move(subscriptionId),
                                std::move(subscription)); });
            return subscriptionId;
        }

        /*! \brief Send a JSON message to a STOMP destination enpoint.
        *
        *  \returns The request ID. If empty, we failed to send the message.
        *
        *  \param destination      The message destination.
        *  \param messageContent   A string containing the message content. We do
        *                          not check if this string is compatible with the
        *                          content type. We assume the content type is
        *                          application/json. The caller must ensure that
        *                          this string stays in scope until the onSend
        *                          handler is called.
        *  \param onSend           This handler is called when the WebSocket
        *                          client terminates the Send operation. On
        *                          success, we cannot guarantee that the message
        *                          reached the STOMP server, only that is was
        *                          correctly sent at the WebSocket level. The
        *                          handler contains an error code and the message
        *                          request ID.
        *
        *  All handlers run in a separate I/O execution context from the WebSocket
        *  one.
        */
        std::string Send(
            const std::string& destination,
            const std::string& messageContent,
            std::function<void (StompClientError, std::string&&)> onSend = nullptr
        )
        {
            spdlog::info("StompClient: Sending message to {}", destination);

            auto requestId {GenerateId()};
            auto messageSize {messageContent.size()};

            // Assemble the SEND frame.
            StompError error {};
            StompFrame frame {
                error,
                StompCommand::kSend,
                {
                    {StompHeader::kId, requestId},
                    {StompHeader::kDestination, destination},
                    {StompHeader::kContentType, "application/json"},
                    {StompHeader::kContentLength, std::to_string(messageSize)},
                },
                messageContent,
            };
            if (error != StompError::kOk) {
                spdlog::error("StompClient: Could not create a valid frame: {}",
                            error);
                return "";
            }

            // Send the WebSocket message.
            if (onSend == nullptr) {
                wsClient.Send(frame.ToString());
            } else {
                wsClient.Send(
                    frame.ToString(),
                    [requestId, onSend](auto ec) mutable {
                        auto error {ec ? StompClientError::kCouldNotSendMessage :
                                        StompClientError::kOk};
                        onSend(error, std::move(requestId));
                    }
                );
            }
            return requestId;
        }

    private:
        // This strand handles all the STOMP subscription messages. These operations
        // are decoupled from the WebSocket operations.
        // We leave it uninitialized because it does not support a default
        // constructor.
        boost::asio::strand<boost::asio::io_context::executor_type>
            m_context;
        WsClient wsClient;
        const std::string& m_url;
        std::string m_username{"mheshamg@gmail.com"};
        std::string m_password{};

        std::function<void(StompClientError)> m_onConnect{nullptr};
        std::function<
            void (StompClientError, const std::string&, std::string&&)
        > m_onMessage{nullptr};
        std::function<void(StompClientError)> m_onDisconnect{nullptr};

        struct Subscription
        {
            std::string destination{};
            std::function<void(
                StompClientError,
                std::string &&)>
                onSubscribe{nullptr};
            std::function<void(
                StompClientError,
                std::string &&)>
                onMessage{nullptr};
        };

        std::unordered_map<std::string, Subscription> m_subscriptions{};

        void onWebSocketConnect(boost::system::error_code ec)
        {
            if (ec)
            {
                spdlog::error("StompClient: Could not connect to server: {}",
                              ec.message());
                if (m_onConnect)
                {
                    boost::asio::post(
                        m_context,
                        [onConnect = m_onConnect]()
                        {
                            onConnect(StompClientError::kCouldNotConnectToWebSocketServer);
                        });
                }
                return;
            }
            StompError frameError;
            StompFrame connectFrame = {frameError,
                                       StompCommand::kStomp,
                                       {{StompHeader::kAcceptVersion, "1.2"},
                                        {StompHeader::kHost, m_url},
                                        {StompHeader::kLogin, m_username},
                                        {StompHeader::kPasscode, m_password}}};
            if (frameError != StompError::kOk)
            {
                spdlog::error("StompClient: Could not create a valid frame: {}",
                              frameError);
                if (m_onConnect)
                {
                    boost::asio::post(
                        m_context,
                        [onConnect = m_onConnect]()
                        {
                            onConnect(StompClientError::kUnexpectedCouldNotCreateValidFrame);
                        });
                }
                return;
            }
            wsClient.Send(connectFrame.ToString(), [this](auto ec)
                          { onWebSocketSendConnectionFrame(ec); });
        }

        void onWebSocketSendConnectionFrame(boost::system::error_code ec)
        {
            if (ec)
            {
                spdlog::error("StompClient: Could not send STOMP frame: {}",
                              ec.message());
                if (m_onConnect)
                {
                    boost::asio::post(
                        m_context,
                        [onConnect = m_onConnect]()
                        {
                            onConnect(StompClientError::kCouldNotSendStompFrame);
                        });
                }
                return;
            }
        }

        void onWebSocketSendSubscribtionFrame(boost::system::error_code ec, std::string &&subscriptionId, Subscription &&subscription)
        {
            if (!ec)
            {
                m_subscriptions.emplace(subscriptionId, std::move(subscription));
            }
            else
            {
                spdlog::error("StompClient: Could not subscribe to {}: {}",
                              subscription.destination, ec.message());
                if (subscription.onSubscribe)
                {
                    boost::asio::post(
                        m_context,
                        [onSubscribe = subscription.onSubscribe]()
                        {
                            onSubscribe(StompClientError::kCouldNotSendSubscribeFrame, "");
                        });
                }
            }
        }

        void onWebSocketReceive(boost::system::error_code ec, std::string&& message)
        {
            StompError error{};
            StompFrame receivedFrame{error, std::move(message)};

            if (error != StompError::kOk)
            {
                spdlog::error(
                    "StompClient: Could not parse message as STOMP frame: {}",
                    error);
                if (m_onConnect)
                {
                    boost::asio::post(
                        m_context,
                        [onConnect = m_onConnect]()
                        {
                            onConnect(StompClientError::kCouldNotParseMessageAsStompFrame);
                        });
                }
                return;
            }

            spdlog::debug("StompClient: Received {}", receivedFrame.GetCommand());
            switch (receivedFrame.GetCommand())
            {
            case StompCommand::kConnected:
            {
                handleConnectedFrame(std::move(receivedFrame));
                break;
            }
            case StompCommand::kError:
            {
                handleError(std::move(receivedFrame));
                break;
            }
            case StompCommand::kReceipt:
            {
                handleSubscriptionReceipt(std::move(receivedFrame));
                break;
            }
            case StompCommand::kMessage:
            {
                handleSubscriptionMessage(std::move(receivedFrame));
                break;
            }
            case StompCommand::kSend: {
                HandleMessage(std::move(receivedFrame));
                break;
            }
            default:
            {
                spdlog::error("StompClient: Unexpected STOMP command: {}",
                              receivedFrame.GetCommand());
                break;
            }
            }
        }

        void onWebSocketDisconnect(boost::system::error_code ec)
        {
            if (m_onDisconnect)
            {
                auto error{ec ? StompClientError::kWebSocketServerDisconnected : StompClientError::kOk};
                boost::asio::post(
                    m_context,
                    [onDisconnect = m_onDisconnect, error]()
                    {
                        onDisconnect(error);
                    });
            }
        }

        void OnWsClose(
            boost::system::error_code ec,
            std::function<void(StompClientError)> onClose = nullptr)
        {
            // Notify the user.
            if (onClose)
            {
                auto error{ec ? StompClientError::kCouldNotCloseWebSocketConnection : StompClientError::kOk};
                boost::asio::post(
                    m_context,
                    [onClose, error]()
                    {
                        onClose(error);
                    });
            }
        }

        void handleConnectedFrame(StompFrame &&frame)
        {
            spdlog::info("StompClient: Successfully connected to STOMP server");
            if (m_onConnect)
            {
                boost::asio::post(
                    m_context,
                    [onConnect = m_onConnect]()
                    {
                        onConnect(StompClientError::kOk);
                    });
            }
        }

        void handleError(StompFrame &&frame)
        {
            spdlog::error("StompClient: The STOMP server returned an error: {}",
                          frame.GetBody());
        }

        void handleSubscriptionReceipt(StompFrame &&frame)
        {
            auto subscriptionId{frame.GetHeaderValue(StompHeader::kReceiptId)};
            auto subscriptionIt{m_subscriptions.find(std::string(subscriptionId))};

            if (subscriptionIt == m_subscriptions.end())
            {
                spdlog::error("StompClient: Cannot find subscription {}",
                              subscriptionId);
                return;
            }

            const auto &subscription{subscriptionIt->second};
            // Notify the user of the susccessful subscription.
            spdlog::info("StompClient: Successfully subscribed to {}",
                         subscriptionId);
            if (subscription.onSubscribe)
            {
                boost::asio::post(
                    m_context,
                    [onSubscribe = subscription.onSubscribe,
                     subscriptionId = std::string(subscriptionId)]() mutable
                    {
                        onSubscribe(StompClientError::kOk,
                                    std::move(subscriptionId));
                    });
            }
        }

        void handleSubscriptionMessage(StompFrame &&frame)
        {
            auto subscriptionId{frame.GetHeaderValue(StompHeader::kSubscription)};
            auto subscriptionIt{m_subscriptions.find(std::string(subscriptionId))};
            if (subscriptionIt == m_subscriptions.end())
            {
                spdlog::error("StompClient: Cannot find subscription {}",
                              subscriptionId);
                return;
            }

            const auto &subscription{subscriptionIt->second};
            auto destination{frame.GetHeaderValue(StompHeader::kDestination)};
            if (destination != subscription.destination)
            {
                spdlog::error("StompClient: Destination mismatch {} / {}",
                              destination, subscription.destination);
                if (subscription.onMessage)
                {
                    boost::asio::post(
                        m_context,
                        [onMessage = subscription.onMessage]()
                        {
                            onMessage(
                                StompClientError::kUnexpectedSubscriptionMismatch,
                                {});
                        });
                }
                return;
            }
            if (subscription.onMessage)
            {
                boost::asio::post(
                    m_context,
                    [onMessage = subscription.onMessage,
                     message = std::string(frame.GetBody())]() mutable
                    {
                        onMessage(StompClientError::kOk, std::move(message));
                    });
            }
        }

        void HandleMessage(StompFrame&& frame)
        {
            // Call the user callback.
            if (m_onMessage) {
                boost::asio::post(
                    m_context,
                    [
                        onMessage = m_onMessage,
                        destination = std::string(frame.GetHeaderValue(
                            StompHeader::kDestination
                        )),
                        message = std::string(frame.GetBody())
                    ]() mutable {
                        onMessage(
                            StompClientError::kOk,
                            destination,
                            std::move(message)
                        );
                    }
                );
            }
        }

        std::string GenerateId()
        {
                std::stringstream ss{};
                ss << boost::uuids::random_generator()();
                return ss.str();
            }
        };

    using BoostStompClient = StompClient<BoostWebSocketClient>;

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_STOMP_CLIENT_H