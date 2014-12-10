#include "LedBuzzerImpl.hpp"
#include "tools/log.hpp"

using namespace Leosac::Module::LedBuzzer;

LedBuzzerImpl::LedBuzzerImpl(zmqpp::context &ctx,
        std::string const &led_name,
        std::string const &gpio_name,
        int blink_duration,
        int blink_speed) :
        ctx_(ctx),
        frontend_(ctx, zmqpp::socket_type::rep),
        backend_(ctx, zmqpp::socket_type::req),
        gpio_(ctx, gpio_name),
        want_update_(false),
        default_blink_duration_(blink_duration),
        default_blink_speed_(blink_speed),
        blink_speed_(0),
        blink_duration_(0),
        blink_count_(0)
{
    frontend_.bind("inproc://" + led_name);
    backend_.connect("inproc://" + gpio_name);
}

zmqpp::socket &LedBuzzerImpl::frontend()
{
    return frontend_;
}

void LedBuzzerImpl::handle_message()
{
    zmqpp::message_t msg;
    zmqpp::message_t rep;
    std::string frame1;

    frontend_.receive(msg);
    msg >> frame1;
    bool ok = false;
    if (frame1 == "STATE")
    {
        return send_state();
    }
    if (frame1 == "ON" || frame1 == "OFF" || frame1 == "TOGGLE")
    {
        // simply forward message to GPIO
        rep = send_to_backend(msg);
        frontend_.send(rep);
        return;
    }
    else if (frame1 == "BLINK")
        ok = start_blink(&msg);
    else // invalid cmd
        assert(0);
    frontend_.send(ok ? "OK" : "KO");
}

void LedBuzzerImpl::update()
{
    DEBUG("UPDATING LED");
    gpio_.toggle();
    next_update_time_ = std::chrono::system_clock::now() + std::chrono::milliseconds(blink_speed_);

    if (--blink_count_ == 0)
    {
        want_update_ = false;
        return;
    }
}

std::chrono::system_clock::time_point LedBuzzerImpl::next_update()
{
    if (want_update_)
        return next_update_time_;
    return std::chrono::system_clock::time_point::max();
}

zmqpp::message LedBuzzerImpl::send_to_backend(zmqpp::message &msg)
{
    zmqpp::message rep;
    backend_.send(msg);

    backend_.receive(rep);
    return rep;
}

bool LedBuzzerImpl::start_blink(zmqpp::message *msg)
{
    std::string tmp;

    if (msg->parts() > 1)
    {
        *msg >> blink_duration_;
    }
    else
    {
        blink_duration_ = default_blink_duration_;
    }

    if (msg->parts() > 2)
    {
        *msg >> blink_speed_;
    }
    else
    {
        blink_speed_ = default_blink_speed_;
    }

    assert(blink_speed_ <= blink_duration_);
    blink_count_ = blink_duration_ / blink_speed_;

    next_update_time_ = std::chrono::system_clock::now() + std::chrono::milliseconds(blink_speed_);
    want_update_ = true;
    gpio_.toggle();
    --blink_count_;
    return true;
}

void LedBuzzerImpl::send_state()
{
    zmqpp::message st;
    if (want_update_)
    {
        // means we are blinking
        st << "BLINKING";
        st << blink_duration_ << blink_speed_;
    }
    st << (gpio_.isOn() ? "ON" : "OFF");
    frontend_.send(st);
}