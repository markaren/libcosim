#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

#include <boost/fiber/future.hpp>

#include <cse/async_slave.hpp>
#include <cse/event_loop.hpp>
#include <cse/libevent.hpp>

#include "mock_slave.hpp"


// A helper macro to test various assertions
#define REQUIRE(test) \
    if (!(test)) throw std::runtime_error("Requirement not satisfied: " #test)


int main()
{
    using boost::fibers::future;

    // Create an event loop and make it fiber friendly.
    std::shared_ptr<cse::event_loop> eventLoop = cse::make_libevent_event_loop();
    cse::event_loop_fiber eventLoopFiber(eventLoop);

    try {
        constexpr int numSlaves = 10;
        constexpr double startTime = 0.0;
        constexpr double endTime = 1.0;
        constexpr double stepSize = 0.1;

        // Create the slaves
        std::vector<std::unique_ptr<cse::async_slave>> asyncSlaves;
        for (int i = 0; i < numSlaves; ++i) {
            asyncSlaves.push_back(make_async_slave<mock_slave>(eventLoop));
        }

        // Get model descriptions from all slaves
        std::vector<future<cse::model_description>> modelDescriptions;
        for (const auto& slave : asyncSlaves) {
            modelDescriptions.push_back(slave->model_description());
        }
        for (auto& md : modelDescriptions) {
            REQUIRE(md.get().name == "mock_slave");
        }

        // Setup all slaves
        std::vector<future<void>> setupResults;
        for (const auto& slave : asyncSlaves) {
            setupResults.push_back(
                slave->setup("", "", startTime, endTime, false, 0.0));
        }
        for (auto& r : setupResults) {
            r.get(); // Throws if an operation failed
        }

        // Start simulation
        std::vector<future<void>> startResults;
        for (const auto& slave : asyncSlaves) {
            startResults.push_back(slave->start_simulation());
        }
        for (auto& r : startResults) {
            r.get();
        }

        // Simulation
        for (double t = startTime; t <= endTime; t += stepSize) {
            // Perform time steps
            std::vector<future<cse::step_result>> stepResults;
            for (const auto& slave : asyncSlaves) {
                stepResults.push_back(slave->do_step(t, stepSize));
            }
            for (auto& r : stepResults) {
                REQUIRE(r.get() == cse::step_result::complete);
            }

            // Get variable values. For now, we simply get the value of each
            // slave's sole real output variable.
            std::vector<future<cse::async_slave::variable_values>> getResults;
            for (const auto& slave : asyncSlaves) {
                const cse::variable_index realOutIndex = 0;
                getResults.push_back(
                    slave->get_variables({&realOutIndex, 1}, {}, {}, {}));
            }
            std::vector<double> values; // To be filled with one value per slave
            for (auto& r : getResults) {
                values.push_back(r.get().real[0]);
            }

            // Set variable values. We connect the slaves such that slave N's
            // input is assigned slave N+1's output, simply by rotating the
            // value vector elements.
            std::rotate(values.begin(), values.begin() + 1, values.end());

            std::vector<future<void>> setResults;
            for (int i = 0; i < numSlaves; ++i) {
                const cse::variable_index realOutIndex = 0;
                setResults.push_back(
                    asyncSlaves[i]->set_variables(
                        {&realOutIndex, 1}, {&values[i], 1},
                        {}, {},
                        {}, {},
                        {}, {}));
            }
            for (auto& r : setResults) {
                r.get();
            }
        }

        // End simulation
        std::vector<future<void>> endResults;
        for (const auto& slave : asyncSlaves) {
            endResults.push_back(slave->end_simulation());
        }
        for (auto& r : endResults) {
            r.get();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
