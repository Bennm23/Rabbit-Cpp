#ifndef RABBIT_CHANNEL_H
#define RABBIT_CHANNEL_H

#include <iostream>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <functional>


typedef std::function<void(void*, size_t, void*)> MsgCallback;

namespace RabbitChannel {

    class RabbitMQChannel {

    public:

        RabbitMQChannel(const std::string &clientName, MsgCallback callback, void* callbackClass);


        void subscribe(const std::string &topic);
        void publish(const std::string &topic, const std::string &message) const;

        void runInForeground();
        void runInBackground();

    private:

        void* _callbackClass;
        MsgCallback _msgCallback;
        const std::string _clientName;
        const std::string _consumerQueue;


        amqp_connection_state_t _connectionStatus;
        amqp_socket_t *_socket = nullptr;
        amqp_channel_t _publishChannel{1};
        amqp_channel_t _consumeChannel{2};


        static int PORT;
        static std::string CONN_IP;
        static std::string USER;
        static std::string PASS;
        static std::string EXCHANGE;
        static std::string VHOST;


        void connect();
        void setupQueue();


    };
};



#endif