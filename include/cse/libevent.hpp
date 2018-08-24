/**
 *  \file
 *  Event loop implementation based on libevent.
 */
#ifndef CSE_LIBEVENT_HPP
#define CSE_LIBEVENT_HPP

#include <memory>
#include <cse/event_loop.hpp>


namespace cse
{


/**
 *  Creates an event loop object whose implementation is based on libevent.
 *
 *  \warning
 *      This function is most likely temporary, and will eventually be
 *      replaced by a factory class.
 */
std::unique_ptr<event_loop> make_libevent_event_loop();


} // namespace
#endif // header guard