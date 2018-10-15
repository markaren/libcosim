/**
 *  \file
 *  \brief Types and functions for running an execution.
 */
#ifndef CSE_EXECUTION_HPP
#define CSE_EXECUTION_HPP

#include <memory>

#include <boost/fiber/future.hpp>

#include <cse/async_slave.hpp>
#include <cse/model.hpp>


namespace cse
{

/// An index which identifies a sub-simulator in an execution.
using simulator_index = int;

/// An index which identifies an observer in an execution.
using observer_index = int;

/// An number which identifies a specific time step in an execution.
using step_number = long long;

/// An object which uniquely identifies a variable in a simulation.
struct variable_id
{
    /// The simulator that owns the variable.
    simulator_index simulator;

    /// The variable data type.
    variable_type type;

    /// The variable index.
    variable_index index;
};

/// Equality operator for `variable_id`.
inline bool operator==(const variable_id& a, const variable_id& b) noexcept
{
    return a.simulator == b.simulator
        && a.type == b.type
        && a.index == b.index;
}

/// Inequality operator for `variable_id`.
inline bool operator!=(const variable_id& a, const variable_id& b) noexcept
{
    return !operator==(a, b);
}


// Forward declarations
class algorithm;
class observer;


/**
 *  A class which represents an execution, i.e., a co-simulation run.
 *
 *  The `execution` class manages all the entities involved in an execution
 *  and provides a high-level API for driving the co-simulation algorithm
 *  forward.
 */
class execution
{
public:
    /**
     *  Constructor.
     *
     *  \param startTime
     *      The logical time at which the simulation will start.
     *  \param algo
     *      The co-simulation algorithm which will be used. One `algorithm`
     *      object may only be used with one `execution`.
     */
    execution(time_point startTime, std::shared_ptr<algorithm> algo);

    ~execution() noexcept;

    execution(const execution&) = delete;
    execution& operator=(const execution&) = delete;

    execution(execution&&) noexcept;
    execution& operator=(execution&&) noexcept;

    /**
     *  Adds a slave to the execution.
     *
     *  \param slave
     *      An object that provides asynchronous communication with the slave.
     *  \param name
     *      An execution-specific name for the slave.
     */
    simulator_index add_slave(
        std::unique_ptr<async_slave> slave,
        std::string_view name);

    observer_index add_observer(std::shared_ptr<observer> obs);

    /**
     *  Connects an output variable to an input variable.
     *
     *  After this, the values of the output variable will be passed to the
     *  input value at the co-simulation algorithm's discretion.  Different
     *  algorithms may handle this in different ways, and could for instance
     *  choose to extrapolate or correct the variable value during transfer.
     */
    void connect_variables(variable_id output, variable_id input);

    /// Returns the current logical time.
    time_point current_time() const noexcept;

    /**
     *  Advance the co-simulation forward to the given logical time.
     *
     *  This function returns immediately, and its actions will be performed
     *  asynchronously.  As it is not possible to perform more than one
     *  asynchronous operation at a time per `execution` object, client code
     *  must verify that the operation completed before calling the function
     *  again (e.g. by calling `boost::fibers::future::get()` on the result).
     *
     *  \param targetTime
     *      The logical time at which the co-simulation should pause.
     *      This must always be greater than the value of `current_time()`
     *      at the moment the function is called.
     *
     *  \returns
     *      `true` if the co-simulation was advanced to the given time,
     *      or `false` if it was stopped before this. In the latter case,
     *      `current_time()` may be called to determine the actual end time.
     */
    boost::fibers::future<bool> simulate_until(time_point targetTime);

    void stop_simulation();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


} // namespace cse
#endif // header guard