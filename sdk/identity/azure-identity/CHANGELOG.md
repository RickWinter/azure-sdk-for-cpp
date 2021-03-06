# Release History

## 1.0.0-beta.5 (Unreleased)


## 1.0.0-beta.4 (2021-03-11)

### New Features

- Added `Azure::Identity::PackageVersion`.

### Breaking Changes

- Removed `TransportPolicyOptions` from `ClientSecretCredentialOptions`. Updated the options to derive from ClientOptions.

## 1.0.0-beta.3 (2021-02-02)

### Breaking Changes

- `ClientSecretCredential ` constructor takes `ClientSecretCredentialOptions` struct instead of authority host string. `TokenCredentialOptions` struct has authority host string as data member.

## 1.0.0-beta.2 (2021-01-13)

### Breaking Changes

- Moved `Azure::Identity::Version`, defined in `azure/identity/version.hpp` to the `Azure::Identity::Details` namespace.

### Other changes and Improvements

- Add high-level and simplified identity.hpp file for simpler include experience for customers.

## 1.0.0-beta.1 (2020-11-11)

### New Features

- Support for Client Secret Credential.
- Support for Environment Credential.
