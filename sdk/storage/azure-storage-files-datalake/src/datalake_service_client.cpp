// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "azure/storage/files/datalake/datalake_service_client.hpp"

#include <azure/core/http/policies/policy.hpp>
#include <azure/storage/blobs/protocol/blob_rest_client.hpp>
#include <azure/storage/common/constants.hpp>
#include <azure/storage/common/shared_key_policy.hpp>
#include <azure/storage/common/storage_common.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include <azure/storage/common/storage_per_retry_policy.hpp>
#include <azure/storage/common/storage_switch_to_secondary_policy.hpp>

#include "azure/storage/files/datalake/datalake_file_system_client.hpp"
#include "azure/storage/files/datalake/datalake_utilities.hpp"
#include "azure/storage/files/datalake/version.hpp"

namespace Azure { namespace Storage { namespace Files { namespace DataLake {

  namespace {
    std::vector<Models::FileSystemItem> FileSystemsFromContainerItems(
        std::vector<Blobs::Models::BlobContainerItem> items)
    {
      std::vector<Models::FileSystemItem> fileSystems;
      for (auto& item : items)
      {
        Models::FileSystemItem fileSystem;
        fileSystem.Name = std::move(item.Name);
        fileSystem.Details.ETag = std::move(item.Details.ETag);
        fileSystem.Details.LastModified = std::move(item.Details.LastModified);
        fileSystem.Details.Metadata = std::move(item.Details.Metadata);
        if (item.Details.AccessType == Blobs::Models::PublicAccessType::BlobContainer)
        {
          fileSystem.Details.AccessType = Models::PublicAccessType::FileSystem;
        }
        else if (item.Details.AccessType == Blobs::Models::PublicAccessType::Blob)
        {
          fileSystem.Details.AccessType = Models::PublicAccessType::Path;
        }
        else if (item.Details.AccessType == Blobs::Models::PublicAccessType::None)
        {
          fileSystem.Details.AccessType = Models::PublicAccessType::None;
        }
        else
        {
          fileSystem.Details.AccessType
              = Models::PublicAccessType(item.Details.AccessType.ToString());
        }
        fileSystem.Details.HasImmutabilityPolicy = item.Details.HasImmutabilityPolicy;
        fileSystem.Details.HasLegalHold = item.Details.HasLegalHold;
        if (item.Details.LeaseDuration.HasValue())
        {
          fileSystem.Details.LeaseDuration
              = Models::LeaseDurationType((item.Details.LeaseDuration.GetValue().ToString()));
        }
        fileSystem.Details.LeaseState = Models::LeaseStateType(item.Details.LeaseState.ToString());
        fileSystem.Details.LeaseStatus
            = Models::LeaseStatusType(item.Details.LeaseStatus.ToString());

        fileSystems.emplace_back(std::move(fileSystem));
      }
      return fileSystems;
    }
  } // namespace

  DataLakeServiceClient DataLakeServiceClient::CreateFromConnectionString(
      const std::string& connectionString,
      const DataLakeClientOptions& options)
  {
    auto parsedConnectionString = Azure::Storage::_detail::ParseConnectionString(connectionString);
    auto serviceUrl = std::move(parsedConnectionString.DataLakeServiceUrl);

    if (parsedConnectionString.KeyCredential)
    {
      return DataLakeServiceClient(
          serviceUrl.GetAbsoluteUrl(), parsedConnectionString.KeyCredential, options);
    }
    else
    {
      return DataLakeServiceClient(serviceUrl.GetAbsoluteUrl(), options);
    }
  }

  DataLakeServiceClient::DataLakeServiceClient(
      const std::string& serviceUrl,
      std::shared_ptr<StorageSharedKeyCredential> credential,
      const DataLakeClientOptions& options)
      : m_serviceUrl(serviceUrl), m_blobServiceClient(
                                      _detail::GetBlobUrlFromUrl(serviceUrl),
                                      credential,
                                      _detail::GetBlobClientOptions(options))
  {
    DataLakeClientOptions newOptions = options;
    newOptions.PerRetryPolicies.emplace_back(
        std::make_unique<Storage::_detail::SharedKeyPolicy>(credential));

    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perRetryPolicies;
    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perOperationPolicies;
    perRetryPolicies.emplace_back(
        std::make_unique<Storage::_detail::StorageSwitchToSecondaryPolicy>(
            m_serviceUrl.GetHost(), newOptions.SecondaryHostForRetryReads));
    perRetryPolicies.emplace_back(std::make_unique<Storage::_detail::StoragePerRetryPolicy>());
    {
      Azure::Core::Http::Policies::_internal::ValueOptions valueOptions;
      valueOptions.HeaderValues[Storage::_detail::HttpHeaderXMsVersion] = newOptions.ApiVersion;
      perOperationPolicies.emplace_back(
          std::make_unique<Azure::Core::Http::Policies::_internal::ValuePolicy>(valueOptions));
    }
    m_pipeline = std::make_shared<Azure::Core::Http::_internal::HttpPipeline>(
        newOptions,
        Storage::_detail::FileServicePackageName,
        PackageVersion::VersionString(),
        std::move(perRetryPolicies),
        std::move(perOperationPolicies));
  }

  DataLakeServiceClient::DataLakeServiceClient(
      const std::string& serviceUrl,
      std::shared_ptr<Core::Credentials::TokenCredential> credential,
      const DataLakeClientOptions& options)
      : m_serviceUrl(serviceUrl), m_blobServiceClient(
                                      _detail::GetBlobUrlFromUrl(serviceUrl),
                                      credential,
                                      _detail::GetBlobClientOptions(options))
  {
    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perRetryPolicies;
    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perOperationPolicies;
    perRetryPolicies.emplace_back(
        std::make_unique<Storage::_detail::StorageSwitchToSecondaryPolicy>(
            m_serviceUrl.GetHost(), options.SecondaryHostForRetryReads));
    perRetryPolicies.emplace_back(std::make_unique<Storage::_detail::StoragePerRetryPolicy>());
    {
      Azure::Core::Credentials::TokenRequestContext tokenContext;
      tokenContext.Scopes.emplace_back(Storage::_detail::StorageScope);
      perRetryPolicies.emplace_back(
          std::make_unique<Azure::Core::Http::Policies::BearerTokenAuthenticationPolicy>(
              credential, tokenContext));
    }
    {
      Azure::Core::Http::Policies::_internal::ValueOptions valueOptions;
      valueOptions.HeaderValues[Storage::_detail::HttpHeaderXMsVersion] = options.ApiVersion;
      perOperationPolicies.emplace_back(
          std::make_unique<Azure::Core::Http::Policies::_internal::ValuePolicy>(valueOptions));
    }
    m_pipeline = std::make_shared<Azure::Core::Http::_internal::HttpPipeline>(
        options,
        Storage::_detail::FileServicePackageName,
        PackageVersion::VersionString(),
        std::move(perRetryPolicies),
        std::move(perOperationPolicies));
  }

  DataLakeServiceClient::DataLakeServiceClient(
      const std::string& serviceUrl,
      const DataLakeClientOptions& options)
      : m_serviceUrl(serviceUrl), m_blobServiceClient(
                                      _detail::GetBlobUrlFromUrl(serviceUrl),
                                      _detail::GetBlobClientOptions(options))
  {
    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perRetryPolicies;
    std::vector<std::unique_ptr<Azure::Core::Http::Policies::HttpPolicy>> perOperationPolicies;
    perRetryPolicies.emplace_back(
        std::make_unique<Storage::_detail::StorageSwitchToSecondaryPolicy>(
            m_serviceUrl.GetHost(), options.SecondaryHostForRetryReads));
    perRetryPolicies.emplace_back(std::make_unique<Storage::_detail::StoragePerRetryPolicy>());
    {
      Azure::Core::Http::Policies::_internal::ValueOptions valueOptions;
      valueOptions.HeaderValues[Storage::_detail::HttpHeaderXMsVersion] = options.ApiVersion;
      perOperationPolicies.emplace_back(
          std::make_unique<Azure::Core::Http::Policies::_internal::ValuePolicy>(valueOptions));
    }
    m_pipeline = std::make_shared<Azure::Core::Http::_internal::HttpPipeline>(
        options,
        Storage::_detail::FileServicePackageName,
        PackageVersion::VersionString(),
        std::move(perRetryPolicies),
        std::move(perOperationPolicies));
  }

  DataLakeFileSystemClient DataLakeServiceClient::GetFileSystemClient(
      const std::string& fileSystemName) const
  {
    auto builder = m_serviceUrl;
    builder.AppendPath(Storage::_detail::UrlEncodePath(fileSystemName));
    return DataLakeFileSystemClient(
        builder, m_blobServiceClient.GetBlobContainerClient(fileSystemName), m_pipeline);
  }

  Azure::Response<Models::ListFileSystemsSinglePageResult>
  DataLakeServiceClient::ListFileSystemsSinglePage(
      const ListFileSystemsSinglePageOptions& options,
      const Azure::Core::Context& context) const
  {
    Blobs::ListBlobContainersSinglePageOptions blobOptions;
    blobOptions.Include = options.Include;
    blobOptions.Prefix = options.Prefix;
    blobOptions.ContinuationToken = options.ContinuationToken;
    blobOptions.PageSizeHint = options.PageSizeHint;
    auto result = m_blobServiceClient.ListBlobContainersSinglePage(blobOptions, context);
    auto response = Models::ListFileSystemsSinglePageResult();
    response.ContinuationToken = std::move(result->ContinuationToken);
    response.RequestId = std::move(result->RequestId);
    response.ServiceEndpoint = std::move(result->ServiceEndpoint);
    response.Prefix = std::move(result->Prefix);
    response.Items = FileSystemsFromContainerItems(std::move(result->Items));
    return Azure::Response<Models::ListFileSystemsSinglePageResult>(
        std::move(response), result.ExtractRawResponse());
  }

}}}} // namespace Azure::Storage::Files::DataLake
