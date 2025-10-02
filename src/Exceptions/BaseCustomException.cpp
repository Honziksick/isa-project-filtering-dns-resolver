/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         BaseCustomException.cpp                                       *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Implementation file for the `BaseCustomException` class       *
 *               used in the Filtering DNS Resolver.                           *
 *                                                                             *
 ******************************************************************************/
/**
 * @file BaseCustomException.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation file for the `BaseCustomException` class used in the
 *        Filtering DNS Resolver.
 */

#include "Exceptions/BaseCustomException.hpp"
#include "Utilities/CastUtils.hpp"
#include <string>   // std::string
#include <utility>  // std::move

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::Exceptions
{
    BaseCustomException::BaseCustomException(const ExitCodes code, string message,
                                             string detail) noexcept
        : mCode{code},
          mMessage{move(message)},
          mDetail{move(detail)} {}

    const char *BaseCustomException::what() const noexcept {
        return mMessage.c_str();
    } // BaseCustomException::what()

    int BaseCustomException::code() const noexcept {
        return CastUtils::castEnumToInt(mCode);
    } // BaseCustomException::code()

    string BaseCustomException::detail() const noexcept {
        return mDetail;
    } // BaseCustomException::detail()
} // FilteringDnsResolver::Exceptions

/*** end of file BaseCustomException.cpp ***/
