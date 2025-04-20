# Workflow Secrets Configuration

This document outlines the secrets required for the GitHub Actions workflows in this repository.

## Required Secrets

The following secrets should be configured in your GitHub repository:

| Secret Name | Description | Used For |
|-------------|-------------|----------|
| `NPM_TOKEN` | NPM authentication token | Publishing packages to npm |
| `GITHUB_TOKEN` | GitHub token (automatically provided by GitHub Actions) | Creating releases and interacting with GitHub API |
| `NUGET_API_KEY` | NuGet API key | Publishing NuGet packages |
| `SIGNING_KEY` | Code signing private key | Signing released binaries |
| `SIGNING_CERT_PASSWORD` | Password for code signing certificate | Authenticating with the signing certificate |

## Setting Up Secrets

1. Go to your GitHub repository
2. Navigate to "Settings" > "Secrets and variables" > "Actions"
3. Click "New repository secret"
4. Enter the name and value for each required secret
5. Click "Add secret"

## Usage in Workflows

These secrets are referenced in workflows using the `${{ secrets.SECRET_NAME }}` syntax.

Example:
```yaml
- name: Publish to npm
  run: npm publish
  env:
    NODE_AUTH_TOKEN: ${{ secrets.NPM_TOKEN }}
```

## Security Considerations

- Never print or log secrets in your workflows
- Limit the number of people who have access to these secrets
- Rotate your secrets periodically for enhanced security
- Consider using OIDC where possible instead of long-lived tokens
```
