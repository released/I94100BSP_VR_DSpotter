param(
    [string]$VoiceName = '',
    [ValidateRange(-10, 10)][int]$Rate = 0,
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'ReplyPhrases/GeneratePhrases.ps1') @PSBoundParameters
