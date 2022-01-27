/*
* This file is part of JKQ QCEC library which is released under the MIT license.
* See file README.md or go to http://iic.jku.at/eda/research/quantum_verification/ for more information.
*/

#include "checker/dd/applicationscheme/LookaheadApplicationScheme.hpp"

namespace ec {
    std::pair<size_t, size_t> ec::LookaheadApplicationScheme::operator()() {
        assert(internalState != nullptr);
        assert(package != nullptr);

        if (!cached1) {
            // cache the current operation
            op1 = taskManager1.getDD();
            package->incRef(op1);
            cached1 = true;
        }

        if (!cached2) {
            // cache the current operation
            op2 = taskManager2.getInverseDD();
            package->incRef(op2);
            cached2 = true;
        }

        // compute both possible applications and measure the resulting size
        auto       saved = *internalState;
        const auto dd1   = package->multiply(op1, saved);
        const auto size1 = package->size(dd1);
        const auto dd2   = package->multiply(saved, op2);
        const auto size2 = package->size(dd2);

        // greedily chose the smaller resulting decision diagram
        if (size1 <= size2) {
            *internalState = dd1;
            package->decRef(op1);
            cached1 = false;
        } else {
            *internalState = dd2;
            package->decRef(op2);
            cached2 = false;
        }

        // properly track reference counts
        package->incRef(*internalState);
        package->decRef(saved);
        package->garbageCollect();

        if (size1 <= size2) {
            if (taskManager1.finished()) {
                // properly handle any cached operations on exit
                if (cached2) {
                    saved          = *internalState;
                    *internalState = package->multiply(saved, op2);
                    package->incRef(*internalState);
                    package->decRef(saved);
                    package->decRef(op2);
                    cached2 = false;
                    package->garbageCollect();
                }
                return {0U, 0U};
            } else {
                taskManager1.advanceIterator();
            }
        } else {
            if (taskManager2.finished()) {
                // properly handle any cached operations on exit
                if (cached1) {
                    saved          = *internalState;
                    *internalState = package->multiply(op1, saved);
                    package->incRef(*internalState);
                    package->decRef(saved);
                    package->decRef(op1);
                    cached1 = false;
                    package->garbageCollect();
                }
                return {0U, 0U};
            } else {
                taskManager2.advanceIterator();
            }
        }

        // no operations shall be applied by the outer loop in which the application scheme is invoked
        return {0U, 0U};
    }
} // namespace ec