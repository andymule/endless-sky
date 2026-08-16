# Back-compat wrapper used by existing VS Code tasks
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    $Remaining
)
& "$PSScriptRoot\build-windows.ps1" -Config Release @Remaining
exit $LASTEXITCODE
