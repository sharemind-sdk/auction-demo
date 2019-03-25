/*
 * Copyright (C) Cybernetica
 *
 * Research/Commercial License Usage
 * Licensees holding a valid Research License or Commercial License
 * for the Software may use this file according to the written
 * agreement between you and Cybernetica.
 *
 * GNU General Public License Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 3.0 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.  Please review the following information to
 * ensure the GNU General Public License version 3.0 requirements will be
 * met: http://www.gnu.org/copyleft/gpl-3.0.html.
 *
 * For further information, please contact us at sharemind@cyber.ee.
 */

#include <boost/program_options.hpp>
#include <cstdint>
#include <iostream>
#include <LogHard/Backend.h>
#include <LogHard/FileAppender.h>
#include <LogHard/Logger.h>
#include <LogHard/StdAppender.h>
#include <memory>
#include <sharemind/controller/SystemController.h>
#include <sharemind/controller/SystemControllerConfiguration.h>
#include <sharemind/controller/SystemControllerGlobals.h>
#include <sstream>
#include <string>

namespace sm = sharemind;

enum Bidder {
    Alice = 0,
    Bob = 1,
    Undefined
};

const std::string bytecode[] {
    "alice_bid.sb",
    "bob_bid.sb"
};

int main(int argc, char ** argv) {
    std::unique_ptr<sm::SystemControllerConfiguration> config;
    Bidder bidder = Bidder::Undefined;
    std::uint64_t bid;

    try {
        namespace po = boost::program_options;

        po::options_description desc(
            "auction-bid\n"
            "Usage: auction-bid [OPTION]...\n\n"
            "Options");
        desc.add_options()
            ("conf,c", po::value<std::string>(), "Set the configuration file.")
            ("alice,a", "Enter bid as Alice.")
            ("bob,b", "Enter bid as Bob.")
            ("bid", po::value<std::uint64_t>(&bid), "Set the bid amount.")
            ("help", "Print this help");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("alice") && ! vm.count("bob")) {
            bidder = Bidder::Alice;
        }

        if (!vm.count("alice") && vm.count("bob")) {
            bidder = Bidder::Bob;
        }

        if (bidder == Bidder::Undefined) {
            std::cout << "Please choose the identity for bidding.\n\n"
                << desc << std::endl;
            return EXIT_FAILURE;
        }

        if (!vm.count("bid")) {
            std::cout << "Please enter the bid.\n\n"
                << desc << std::endl;
            return EXIT_FAILURE;
        }

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        if (vm.count("conf")) {
            config = std::make_unique<sm::SystemControllerConfiguration>(
                         vm["conf"].as<std::string>());
        } else {
            config = std::make_unique<sm::SystemControllerConfiguration>();
        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto logBackend(std::make_shared<LogHard::Backend>());
    logBackend->addAppender(std::make_shared<LogHard::StdAppender>());
    logBackend->addAppender(
                std::make_shared<LogHard::FileAppender>(
                    "auction-bid.log",
                    LogHard::FileAppender::OVERWRITE));
    const LogHard::Logger logger(logBackend);

    try {
        sm::SystemControllerGlobals systemControllerGlobals;
        sm::SystemController c(logger, *config);

        // Initialize the argument map and set the arguments
        sm::SystemController::ValueMap arguments;

        arguments["bid"] =
            std::make_shared<sm::SystemController::Value>(
                "pd_shared3p",
                "uint64",
                std::make_shared<std::uint64_t>(bid),
                sizeof(std::uint64_t));

        // Run code
        logger.info() << "Sending secret shared arguments and running SecreC bytecode on the servers";
        sm::SystemController::ValueMap results = c.runCode(bytecode[bidder], arguments);


    } catch (const sm::SystemController::WorkerException & e) {
        logger.fatal() << "Multiple exceptions caught:";
        for (size_t i = 0u; i < e.numWorkers(); i++) {
            if (std::exception_ptr ep = e.nested_ptrs()[i]) {
                logger.fatal() << "  Exception from server " << i << ':';
                try {
                    std::rethrow_exception(std::move(ep));
                } catch (...) {
                    logger.printCurrentException<LogHard::Priority::Fatal>(
                            LogHard::Logger::StandardExceptionFormatter(4u));
                }
            }
        }
    } catch (const std::exception & e) {
        logger.error() << "Caught exception: " << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
