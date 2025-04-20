# CI/CD Configuration Guide

## Required Secrets

The GitHub Actions workflows require the following secrets to be set up in your repository settings. Navigate to your repository on GitHub, go to "Settings" > "Secrets and variables" > "Actions" and add the following secrets:

### NPM_TOKEN

Used by the `publish.yml` workflow to authenticate with npm when publishing packages.

To obtain this token:
1. Log in to your npm account at https://www.npmjs.com
2. Click on your profile picture in the top right and select "Access Tokens"
3. Click "Generate New Token" and select "Automation" token type
4. Provide a description like "GitHub Actions Publishing"
5. Copy the generated token (you'll only see it once)
6. Add it as a repository secret named `NPM_TOKEN`

### CODACY_PROJECT_TOKEN

Used by the `codacy.yml` workflow to authenticate with Codacy for code analysis.

To obtain this token:
1. Log in to your Codacy account at https://app.codacy.com
2. Navigate to your project
3. Go to "Settings" > "Integrations" > "Project API"
4. Copy the provided API token
5. Add it as a repository secret named `CODACY_PROJECT_TOKEN`

## Setting Up Repository Secrets

1. Navigate to your repository on GitHub
2. Go to "Settings" > "Secrets and variables" > "Actions"
3. Click "New repository secret"
4. Enter the secret name (e.g., `NPM_TOKEN`)
5. Paste the value
6. Click "Add secret"
7. Repeat for each required secret

## Optional: Adding Codacy Badge

If you're using Codacy for code quality, add the badge to your README.md:

```markdown
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/{YOUR_PROJECT_ID})](https://www.codacy.com/gh/{YOUR_USERNAME}/{YOUR_REPO}/dashboard)
```

Replace `{YOUR_PROJECT_ID}`, `{YOUR_USERNAME}`, and `{YOUR_REPO}` with your actual values.
