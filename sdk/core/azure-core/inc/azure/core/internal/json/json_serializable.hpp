// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 * @brief Internal interface for serializable classes.
 *
 */

#pragma once

#include <string>

namespace Azure { namespace Core { namespace Json { namespace _internal {

  /**
   * @brief Interface for json-serializable components.
   *
   */
  class JsonSerializable {
  public:
    /**
     * @brief Serialize object as json string.
     *
     * @return The json string representation.
     */
    virtual std::string Serialize() const = 0;
  };

}}}} // namespace Azure::Core::Json::_internal
