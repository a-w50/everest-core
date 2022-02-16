// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "auth_token_validatorImpl.hpp"

namespace module {
namespace auth_validator {

void auth_token_validatorImpl::init() {
}

void auth_token_validatorImpl::ready() {
}

::auth_token_validator::ResultType auth_token_validatorImpl::handle_validate_token(std::string& token) {
    auto auth_status = mod->charge_point->authorize_id_tag(token);
    ::auth_token_validator::ResultType result;
    switch (auth_status) {
    case ocpp1_6::AuthorizationStatus::Accepted:
        result.result = ::auth_token_validator::Result::Accepted;
        break;
    case ocpp1_6::AuthorizationStatus::Blocked:
        result.result = ::auth_token_validator::Result::Blocked;
        break;
    case ocpp1_6::AuthorizationStatus::Expired:
        result.result = ::auth_token_validator::Result::Expired;
        break;
    case ocpp1_6::AuthorizationStatus::Invalid:
        result.result = ::auth_token_validator::Result::Invalid;
        break;

    default:
        result.result = ::auth_token_validator::Result::Invalid;
        break;
    }

    result.reason.emplace("Validation by OCPP 1.6 Central System");

    return result;
};

} // namespace auth_validator
} // namespace module
