# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.13)

project (sample3-delete-recover LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable (
  sample3-delete-recover
  sample3_delete_recover.cpp
)

target_compile_definitions(sample3-delete-recover PRIVATE _azure_BUILDING_SAMPLES)

create_per_service_target_build_for_sample(keyvault sample3-delete-recover)

target_link_libraries(sample3-delete-recover PRIVATE azure-security-keyvault-secrets azure-identity get-env-helper)
