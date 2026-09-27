<#
  离线重算：读已存盘的 CSV + .meta.txt，重算阶跃响应指标。
  调参过程中指标口径会反复改，不该每次都去动电机。
#>
param(
    [Parameter(Mandatory=$true)][string]$Csv,
    [string]$Ch,
    [double]$TStep    = [double]::NaN,
    [double]$Baseline = [double]::NaN,
    [double]$Expected = [double]::NaN,
    [double]$LateFrac = 0.25,
    [switch]$NoPlot
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\VofaLib.ps1"

$ci = [Globalization.CultureInfo]::InvariantCulture
$metaPath = $Csv -replace '\.csv$', '.meta.txt'
$meta = @{}
if (Test-Path $metaPath) {
    Get-Content $metaPath | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { $meta[$Matches[1]] = $Matches[2] }
    }
} else {
    Write-Host "找不到 $metaPath —— 需要手动指定 -Ch/-TStep/-Baseline/-Expected" -ForegroundColor Yellow
}

function MetaNum {
    param([string]$Key, [double]$Fallback)
    if ($meta.ContainsKey($Key)) { return [double]::Parse($meta[$Key], $ci) }
    return $Fallback
}

if (-not $Ch)                  { $Ch       = $meta['ch'] }
if ([double]::IsNaN($TStep))    { $TStep    = MetaNum 'tStep'    ([double]::NaN) }
if ([double]::IsNaN($Baseline)) { $Baseline = MetaNum 'baseline' ([double]::NaN) }
if ([double]::IsNaN($Expected)) { $Expected = MetaNum 'expected' ([double]::NaN) }

foreach ($p in @(@('Ch',$Ch), @('tStep',$TStep), @('baseline',$Baseline), @('expected',$Expected))) {
    if ($null -eq $p[1] -or ($p[1] -is [double] -and [double]::IsNaN($p[1]))) {
        Write-Host "缺少参数 $($p[0])，无法分析" -ForegroundColor Red; exit 1
    }
}

$rows = Read-VofaCsv $Csv
$m = Get-StepMetrics -Rows $rows -Ch $Ch -TStep $TStep -Baseline $Baseline `
                     -ExpectedFinal $Expected -LateFrac $LateFrac

Write-Host ("文件 {0}   通道 {1}   帧数 {2}" -f (Split-Path $Csv -Leaf), $Ch, $rows.Count)
if ($meta.ContainsKey('Kp')) {
    Write-Host ("参数 Kp={0} Ki={1} Kd={2} Step={3}" -f $meta['Kp'], $meta['Ki'], $meta['Kd'], $meta['Step'])
}
Write-Host ("基线={0:N6}  期望终值={1:N6}  阶跃={2:N6}  触发 t={3:N4}s" -f $m.Baseline, $m.Expected, $m.Step, $TStep)

if (-not $m.Valid) { Write-Host ("指标无效: {0}" -f $m.Note) -ForegroundColor Red; exit 1 }

Write-Host "`n--- 阶跃响应指标 ---" -ForegroundColor Green
Write-Host ("  上升时间 (10-90%)  : {0}" -f $(if ($null -ne $m.RiseTime) { "{0:N3} ms" -f ($m.RiseTime*1000) } else { "未达到 90%" }))
Write-Host ("  超调量             : {0:N2} %" -f $m.OvershootPct)
Write-Host ("  峰值 / 峰值时刻    : {0:N6}  @ {1:N2} ms" -f $m.Peak, ($m.PeakTime*1000))
Write-Host ("  调节时间           : {0:N1} ms   (稳态带 +-{1:N6})" -f ($m.SettleTime*1000), $m.SettleBand)
Write-Host ("  稳态均值 / 静差    : {0:N6}  ({1:N3} %)" -f $m.SteadyMean, $m.SteadyErrPct)
Write-Host ("  纹波 峰峰 / 标准差 : {0:N6}  /  {1:N6}" -f $m.RipplePP, $m.RippleStd)
if ($m.Note -ne '') { Write-Host ("  注: {0}" -f $m.Note) -ForegroundColor Yellow }

if (-not $NoPlot) {
    Write-Host "`n波形 (竖线 = 阶跃时刻):" -ForegroundColor Cyan
    Show-AsciiPlot $rows $Ch $TStep
}
