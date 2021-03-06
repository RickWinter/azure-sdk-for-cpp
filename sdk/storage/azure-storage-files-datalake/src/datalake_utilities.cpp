// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "azure/storage/files/datalake/datalake_utilities.hpp"

#include <azure/storage/common/crypt.hpp>

#include "azure/storage/files/datalake/datalake_constants.hpp"
#include "azure/storage/files/datalake/protocol/datalake_rest_client.hpp"

namespace Azure { namespace Storage { namespace Files { namespace DataLake { namespace _detail {

  const static std::string DfsEndPointIdentifier = ".dfs.";
  const static std::string BlobEndPointIdentifier = ".blob.";

  std::string GetBlobUrlFromUrl(const std::string& url)
  {
    std::string result = url;
    auto pos = result.find(DfsEndPointIdentifier);
    if (pos != std::string::npos)
    {
      result.replace(pos, DfsEndPointIdentifier.size(), BlobEndPointIdentifier);
    }
    return result;
  }

  std::string GetDfsUrlFromUrl(const std::string& url)
  {
    std::string result = url;
    auto pos = result.find(BlobEndPointIdentifier);
    if (pos != std::string::npos)
    {
      result.replace(pos, BlobEndPointIdentifier.size(), DfsEndPointIdentifier);
    }
    return result;
  }

  std::string SerializeMetadata(const Storage::Metadata& dataLakePropertiesMap)
  {
    std::string result;
    for (const auto& pair : dataLakePropertiesMap)
    {
      result.append(
          pair.first + "="
          + Azure::Core::Convert::Base64Encode(
              std::vector<uint8_t>(pair.second.begin(), pair.second.end()))
          + ",");
    }
    if (!result.empty())
    {
      result.pop_back();
    }
    return result;
  }

  std::string GetSubstringTillDelimiter(
      char delimiter,
      const std::string& string,
      std::string::const_iterator& cur)
  {
    auto begin = cur;
    auto end = std::find(cur, string.end(), delimiter);
    cur = end;
    if (cur != string.end())
    {
      ++cur;
    }
    return std::string(begin, end);
  }

  bool MetadataIncidatesIsDirectory(const Storage::Metadata& metadata)
  {
    auto ite = metadata.find(DataLakeIsDirectoryKey);
    return ite != metadata.end() && ite->second == "true";
  }

  Blobs::BlobClientOptions GetBlobClientOptions(const DataLakeClientOptions& options)
  {
    Blobs::BlobClientOptions blobOptions;
    *(static_cast<Azure::Core::_internal::ClientOptions*>(&blobOptions)) = options;
    blobOptions.SecondaryHostForRetryReads
        = _detail::GetBlobUrlFromUrl(options.SecondaryHostForRetryReads);
    blobOptions.ApiVersion = options.ApiVersion;
    return blobOptions;
  }

}}}}} // namespace Azure::Storage::Files::DataLake::_detail
