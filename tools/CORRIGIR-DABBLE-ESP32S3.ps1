# Corrige ponteiros nao inicializados no parser DabbleESP32. Faz backup.
param([string]$LibraryFile = (Join-Path $HOME 'Documents\Arduino\libraries\DabbleESP32\src\DabbleESP32.cpp'))
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $LibraryFile -PathType Leaf)) {
    throw "Nao encontrei: $LibraryFile. Use -LibraryFile para indicar o arquivo."
}
$source = [System.IO.File]::ReadAllText($LibraryFile)
$oldPtr = 'arguments\s*=\s*new\s+byte\s*\*\s*\[\s*argumentnumber\s*\]\s*;'
$newPtr = 'arguments = new byte*[argumentnumber]();'
$oldLen = 'argumentL\s*=\s*new\s+byte\s*\[\s*argumentnumber\s*\]\s*;'
$newLen = 'argumentL = new byte[argumentnumber]();'
$ptrMatches = [regex]::Matches($source, $oldPtr)
$lenMatches = [regex]::Matches($source, $oldLen)
if ($ptrMatches.Count -eq 0 -and $lenMatches.Count -eq 0 -and $source.Contains($newPtr) -and $source.Contains($newLen)) {
    Write-Host 'A biblioteca ja contem esta correcao.' -ForegroundColor Green
    exit 0
}
if ($ptrMatches.Count -ne 1 -or $lenMatches.Count -ne 1 -or $source -notmatch 'delete\s*\[\s*\]\s*arguments\s*\[\s*i\s*\]') {
    throw 'Versao inesperada da biblioteca. Nenhum arquivo foi alterado.'
}
$backup = "$LibraryFile.bak-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
Copy-Item -LiteralPath $LibraryFile -Destination $backup -ErrorAction Stop
$patched = [regex]::Replace([regex]::Replace($source, $oldPtr, $newPtr), $oldLen, $newLen)
[System.IO.File]::WriteAllText($LibraryFile, $patched, [System.Text.UTF8Encoding]::new($false))
Write-Host 'DabbleESP32 corrigido.' -ForegroundColor Green
Write-Host "Biblioteca: $LibraryFile"
Write-Host "Backup: $backup"
Write-Host 'Reabra o Arduino IDE, compile e grave o firmware novamente.'
