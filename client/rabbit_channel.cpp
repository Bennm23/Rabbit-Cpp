#include "rabbit_channel.h"
#include <thread>

using namespace RabbitChannel;

int RabbitMQChannel::PORT = 5672;
std::string RabbitMQChannel::CONN_IP = "localhost";
std::string RabbitMQChannel::USER = "benn";
std::string RabbitMQChannel::PASS = "benn";
std::string RabbitMQChannel::EXCHANGE = "TestExchange";
std::string RabbitMQChannel::VHOST = "/";


RabbitMQChannel::RabbitMQChannel(const std::string &clientName, MsgCallback callback, void* callbackClass)
    : _msgCallback(callback), _callbackClass(callbackClass), _queueName(clientName + "_consumer")
{
    std::cout << "INIT QUEUE = " << _queueName << std::endl;

    connect();
    setupQueue();
}

void RabbitMQChannel::connect()
{

    _connectionStatus = amqp_new_connection();
    _socket = amqp_tcp_socket_new(_connectionStatus);

    if (!_socket)
    {
        std::cerr << "Failed to create RabbitMQ Socket" << std::endl;
        return;
    }

    if (amqp_socket_open(_socket, CONN_IP.c_str(), PORT))
    {
        std::cerr << "Failed to open RabbitMQ TCP Socket" << std::endl;
        return;
    }

    amqp_rpc_reply_t reply = amqp_login(
        _connectionStatus, VHOST.c_str(), AMQP_DEFAULT_MAX_CHANNELS,
        AMQP_DEFAULT_FRAME_SIZE, AMQP_DEFAULT_HEARTBEAT,
        amqp_sasl_method_enum::AMQP_SASL_METHOD_PLAIN,
        USER.c_str(), PASS.c_str()
    );

    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        std::cerr << "Failed to login: " << amqp_error_string2(reply.reply.id) << std::endl;
    }

    amqp_channel_open(_connectionStatus, _publishChannel);
    reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to open publish channel" << std::endl;
        return;
    }
    amqp_channel_open(_connectionStatus, _consumeChannel);
    reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to open consume channel" << std::endl;
        return;
    }


    //passive - if true throw error if exchange does not exist. if false, create exchange if it doesn't exist
    //durable - exchange survives server restart
    //auto_delete - delete exchange when last queue is unbound
    //internal - if true exchange is only used for routing, consumers can not publish on.
    amqp_exchange_declare(
        _connectionStatus, _publishChannel, amqp_cstring_bytes(EXCHANGE.c_str()), amqp_cstring_bytes("topic"),
        //passive, durable, auto_delete, internal
        false, false, false, false, amqp_empty_table
    );
    reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to declare exchange for queue: " << _queueName << ": " << amqp_error_string2(reply.reply.id) << std::endl;
        return;
    }
}

void RabbitMQChannel::setupQueue()
{
    //passive - if true do nothing if queue does not exist. if false, create queue if it doesn't exist
    //durable - queue survives server restart
    //exclusive - an exclusive queue is a queue that is used only by the connection that created it
    //auto_delete - delete queue when no consumers are attached to it
    amqp_queue_declare(
        _connectionStatus, _consumeChannel, amqp_cstring_bytes(_queueName.c_str()),
        //passive, durable, exclusivem auto_delete 
        false, false, true, true, amqp_empty_table
    );
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to declare queue: " << amqp_error_string2(reply.reply.id) << std::endl;
        return;
    }


}
void RabbitMQChannel::subscribe(const std::string &topic)
{
    amqp_queue_bind(
        _connectionStatus, _consumeChannel, amqp_cstring_bytes(_queueName.c_str()),
        amqp_cstring_bytes(EXCHANGE.c_str()), amqp_cstring_bytes(topic.c_str()), amqp_empty_table);
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to bind queue: " << amqp_error_string2(reply.reply.id) << std::endl;
        return;
    }
}

void RabbitMQChannel::publish(const std::string& topic, const std::string &message) const
{
    //mandatory - if rabbitmq cannot route the message to any queue(no bindings for a routing key) it will return the message to this publisher
    //immediate - return immediately if it can not be delivered to any consumers
    amqp_basic_publish(
        _connectionStatus, _publishChannel, amqp_cstring_bytes(EXCHANGE.c_str()),
        amqp_cstring_bytes(topic.c_str()),
        //mandatory, immediate
        false, false, new amqp_basic_properties_t(),
        amqp_cstring_bytes(message.c_str())
    );

    amqp_rpc_reply_t reply = amqp_get_rpc_reply(_connectionStatus);
    std::cout << "Published Message = " << message << " To Topic = " << topic << std::endl;
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to publish message: " << amqp_error_string2(reply.reply.id) << std::endl;
    }
}

void RabbitMQChannel::runInForeground()
{
    //no-local  - when true prevent consumer from receving messages it published itself
    //no-ack    - when true, auto ack is enabled, else manual acknowledgement is required by consumers
    //exclusive - when true, the queue is only used by this consumer when this consumer is diconnected the queue will be deleted
    amqp_basic_consume(
        _connectionStatus, _consumeChannel,
        amqp_cstring_bytes(_queueName.c_str()), amqp_cstring_bytes(""),
        //no-local, no-ack, exclusive
        true, true, true, amqp_empty_table
    );
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(_connectionStatus);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        std::cerr << "Failed to basic consume, err: " << amqp_error_string2(reply.reply.id) << std::endl;
        return;
    }
    while (true)
    {
        amqp_envelope_t envelope;
        amqp_rpc_reply_t reply = amqp_consume_message(_connectionStatus, &envelope, nullptr, 0);

        if (reply.reply_type == AMQP_RESPONSE_NORMAL)
        {
            _msgCallback(_callbackClass, envelope.message.body.len, envelope.message.body.bytes);
            amqp_destroy_envelope(&envelope);
        }
        else
        {
            std::cerr << "Queue: " << _queueName << " Failed to consume message: " << amqp_error_string2(reply.reply.id) << std::endl;
            return;
        }
    }
}

void RabbitMQChannel::runInBackground()
{
    std::thread{&RabbitMQChannel::runInForeground, this}.detach();
}

// bool RabbitMQManager::startConsuming(const std::string& queueName, std::function<void(const std::string&)> callback) {
    // amqp_basic_consume(conn_, consume_channel_, amqp_cstring_bytes(queueName.c_str()), amqp_cstring_bytes(""), 0, 1, 0, amqp_empty_table);
//     amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
//     if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
//         std::cerr << "Failed to start consuming: " << amqp_error_string2(reply.reply.id) << std::endl;
//         return false;
//     }

//     while (true) {
//         amqp_envelope_t envelope;
//         amqp_rpc_reply_t reply = amqp_consume_message(conn_, &envelope, nullptr, 0);
//         if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
//             std::string message((char*)envelope.message.body.bytes, envelope.message.body.len);
//             callback(message);
//             amqp_destroy_envelope(&envelope);
//         } else {
//             std::cerr << "Failed to consume message: " << amqp_error_string2(reply.reply.id) << std::endl;
//             return false;
//         }
//     }
// }
