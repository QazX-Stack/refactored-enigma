# 只读探测：确认串口是那块板子，不发任何命令
param(
    [string]$Port = "COM7",
    [int]$Baud = 460800,
    [double]$Seconds = 2.0
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\VofaLib.ps1"

$logDir = Join-Path $PSScriptRoot 'logs'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }

$csv = Join-Path $logDir 'probe.csv'

$scope = New-Object VofaScope
try {
    $scope.Open($Port, $Baud)
} catch {
    Write-Host "打开 $Port 失败: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "→ VOFA+ 是不是还开着？或者板子没在跑？" -ForegroundColor Yellow
    exit 1
}

Write-Host "已打开 $Port @ $Baud，采集 $Seconds 秒（未发送任何命令）..." -ForegroundColor Cyan
$n = $scope.Capture($Seconds, $csv)
$scope.Close()

Write-Host ("有效帧: {0}   坏帧: {1}" -f $n, $scope.BadFrames)

if ($n -lt 10) {
    Write-Host "几乎没收到数据 —— 这个口可能不是板子的 USART2，或板子没在运行。" -ForegroundColor Red
    exit 1
}

$rows = Read-VofaCsv $csv
$t0 = $rows[0].t; $t1 = $rows[$rows.Count-1].t
Write-Host ("实测帧率: {0:N4} Hz   (跨度 {1:N3}s / {2} 帧)" -f $scope.MeasuredRate, ($t1-$t0), $rows.Count)
Write-Host ("  TIM6 理论值 1000.0000 Hz  ->  丢帧率约 {0:N3}%" -f ((1000.0 - $scope.MeasuredRate)/10.0))

Write-Host "`n通道统计 (A=Angle.Target B=Angle.Actual C=Omega.Target D=Omega.Actual):" -ForegroundColor Cyan
foreach ($ch in 'A','B','C','D') {
    $v = Get-ChannelValues $rows $ch
    $st = $v | Measure-Object -Minimum -Maximum -Average
    Write-Host ("  {0}: min={1,10:N4}  max={2,10:N4}  mean={3,10:N4}" -f $ch, $st.Minimum, $st.Maximum, $st.Average)
}

Write-Host "`nAngle.Actual (B) 波形:" -ForegroundColor Cyan
Show-AsciiPlot $rows 'B'

Write-Host "`nOmega.Actual (D) 波形:" -ForegroundColor Cyan
Show-AsciiPlot $rows 'D'

Write-Host "`n原始数据已存: $csv" -ForegroundColor Green
