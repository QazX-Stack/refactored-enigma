<#
  离线重算：把已存盘的某一批实验用当前指标口径重新算一遍。
  指标口径会反复调整，这个脚本让每次调整都不需要重新驱动电机。
#>
param(
    [string]$Pattern = 'inner_kp_*',
    [string]$SortBy  = 'Kp',
    [double]$SmoothMs = 5.0
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\TuneLib.ps1"

$ci     = [Globalization.CultureInfo]::InvariantCulture
$LogDir = Join-Path $PSScriptRoot 'logs'

$files = Get-ChildItem (Join-Path $LogDir "$Pattern.csv") -ErrorAction SilentlyContinue
if (-not $files) { Write-Host "没有匹配 $Pattern 的 CSV" -ForegroundColor Red; exit 1 }

$results = New-Object 'System.Collections.Generic.List[object]'
foreach ($f in $files) {
    $metaPath = $f.FullName -replace '\.csv$', '.meta.txt'
    if (-not (Test-Path $metaPath)) { Write-Host "跳过 $($f.Name) (无 meta)" -ForegroundColor DarkGray; continue }

    $meta = @{}
    Get-Content $metaPath | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { $meta[$Matches[1]] = $Matches[2] }
    }

    $rows = Read-VofaCsv $f.FullName
    $m = Get-StepMetrics -Rows $rows -Ch $meta['ch'] `
            -TStep         ([double]::Parse($meta['tStep'],    $ci)) `
            -Baseline      ([double]::Parse($meta['baseline'], $ci)) `
            -ExpectedFinal ([double]::Parse($meta['expected'], $ci)) `
            -SmoothMs $SmoothMs

    $results.Add([pscustomobject]@{
        Tag     = $f.BaseName
        Kp      = [double]::Parse($meta['Kp'], $ci)
        Ki      = [double]::Parse($meta['Ki'], $ci)
        Kd      = [double]::Parse($meta['Kd'], $ci)
        Metrics = $m
    })
}

$sorted = $results | Sort-Object -Property $SortBy
Write-MetricTable $sorted

$best = $sorted | Where-Object { -not [double]::IsPositiveInfinity((Get-InnerCost $_.Metrics)) } |
                  Sort-Object { Get-InnerCost $_.Metrics } | Select-Object -First 1
if ($best) {
    Write-Host ("`n当前口径下最优: Kp={0}  代价 {1:N2}" -f $best.Kp, (Get-InnerCost $best.Metrics)) -ForegroundColor Green
}
