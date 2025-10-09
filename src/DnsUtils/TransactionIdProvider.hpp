/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         TransactionIdProvider.hpp                                     *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `TransactionIdProvider` class,      *
 *               which manages allocation and deallocation of DNS transaction  *
 *               IDs for the resolver. It provides thread-safe generation of   *
 *               unique 16-bit transaction IDs using random number generation  *
 *               and bitmap tracking to prevent ID conflicts in concurrent     *
 *               DNS query processing.                                         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file TransactionIdProvider.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `TransactionIdProvider` class for DNS transaction
 *        ID allocation and management functionality.
 */

#ifndef DNS_ID_PROVIDER_HPP
#define DNS_ID_PROVIDER_HPP

#include <cstdint>  // uint16_t
#include <vector>   // std::vector

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class TransactionIdProvider
     * @brief Manages allocation and deallocation of DNS transaction IDs.
     *
     * @details Provides unique 16-bit transaction ID generation using random
     *          number generation with bitmap tracking to prevent conflicts.
     *          Ensures each allocated ID is unique within the resolver instance
     *          and properly tracks ID lifecycle for reuse after release.
     */
    class TransactionIdProvider final {
    public:
        /**
         * @brief Constructs a transaction ID provider with empty allocation state.
         *
         * @details Initializes the bitmap for tracking used IDs. All transaction
         *          IDs are initially available for allocation.
         */
        TransactionIdProvider();

        static constexpr auto ALL_IDS_USED{0}; /**< Constant returned when all transaction IDs are exhausted */

        /**
         * @brief Allocates the next available transaction ID.
         *
         * @details Generates random 16-bit transaction IDs and returns the first
         *          unused one. Uses bitmap tracking to ensure uniqueness and
         *          automatically marks the returned ID as used.
         *
         * @return Unique 16-bit transaction ID, or ALL_IDS_USED if no IDs available.
         */
        [[nodiscard]]
        uint16_t getNextId() noexcept;

        /**
         * @brief Releases a previously allocated transaction ID for reuse.
         *
         * @details Marks the specified transaction ID as available for future
         *          allocation. The ID can be safely reused after this call.
         *
         * @param id Transaction ID to release back to the available pool.
         */
        void releaseId(uint16_t id) noexcept;

    private:
        std::vector<bool> mUsed;  /**< Bitmap tracking which transaction IDs are currently allocated */

        static constexpr auto MAX_RNG_ATTEMPTS{32};  /**< Maximum attempts to find unused random ID before exhaustion */

        /**
         * @brief Generates a random 16-bit transaction ID.
         *
         * @details Uses system random number generation to create candidate
         *          transaction IDs for allocation attempts.
         *
         * @return Random 16-bit value for potential transaction ID use.
         */
        [[nodiscard]]
        static uint16_t next();

        /**
         * @brief Checks if a transaction ID is currently allocated.
         *
         * @details Queries the internal bitmap to determine if the specified
         *          transaction ID is already in use.
         *
         * @param id Transaction ID to check for current allocation status.
         *
         * @return `true` if ID is allocated, `false` if available.
         */
        [[nodiscard]]
        bool isUsed(uint16_t id) noexcept;

        /**
         * @brief Marks a transaction ID as allocated in the bitmap.
         *
         * @details Updates the internal tracking to indicate the specified
         *          transaction ID is now in use and unavailable for allocation.
         *
         * @param id Transaction ID to mark as allocated.
         */
        void markUsed(uint16_t id) noexcept;
    }; // TransactionIdProvider
} // FilteringDnsResolver::DnsUtils

#endif // DNS_ID_PROVIDER_HPP

/*** end of file TransactionIdProvider.hpp ***/
