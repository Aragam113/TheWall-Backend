$ErrorActionPreference = 'Stop'

# Configure git to use the repo-local hooks directory.
git config core.hooksPath .githooks
Write-Host "core.hooksPath set to .githooks"