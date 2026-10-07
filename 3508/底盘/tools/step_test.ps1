<#
  单次阶跃测试 —— 先验证链路，再往上搭自动搜索

  内环 (inner)：把角度环当成"饱和发生器"用。
    Kp 取很小（1.0）即足够把 Angle.Out 顶到 OutMax，同时对位置抖动
    几乎不敏感，基线干净。这样 Angle.Out 就是一个干净的 ±Step 速度阶跃，
    直接激励速度环 —— 不需要改固件。

  外环 (outer)：内环增益固定，直接对位置下阶跃。
#>
param(
    [ValidateSet('inner', 'outer')] [string]$Loop = 'inner',
    [double]$Kp = 2000,        # 被测环的 Kp
    [double]$Ki = 0,
    [double]$Kd = 0,
    [double]$Step = 5.0,       # inner: 速度阶跃 rad/s ; outer: 位置阶跃 rad
    [double]$OutMax = 16384,   # inner: 力矩限幅 ; outer: 速度限幅 rad/s

    # 仅 outer 用：内环已整定好的增益
    [double]$InnerKp = 2000,
    [double]$InnerKi = 0,
    [double]$InnerKd = 0,

    [string]$Tag = '',
    [double]$Pre = 0.30,
    [double]$Post = 1.00,
    [string]$Port = 'COM7',
    [int]$Baud = 460800
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\VofaLib.ps1"

$LogDir = Join-Path $PSScriptRoot 'logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir | Out-Null }
if ($Tag -eq '') { $Tag = "{0}_Kp{1}_Ki{2}_Kd{3}_S{4}" -f $Loop, $Kp, $Ki, $Kd, $Step }

$scope = New-Object VofaScope
$scope.Open($Port, $Baud)
$tmpCsv = Join-Path $LogDir '_state.csv'
$csv    = Join-Path $LogDir "$Tag.csv"

try {
    Stop-Motor $Scope
    if (-not (Wait-MotorStop $scope $tmpCsv 3.0)) {
        Write-Host "警告：电机没停稳，结果可能不准" -ForegroundColor Yellow
    }

    $st = Read-State $scope $tmpCsv 0.20
    Write-Host ("断力后静止: 位置={0:N5} rad   转速={1:N5} rad/s   转速峰峰={2:N5}" -f $st.Angle, $st.Omega, $st.OmegaPP)

    if ($Loop -eq 'inner') {
        # 角度环 = 饱和发生器
        Send-Cmd $scope @('1P1', '1I0', '1D0',
                          "1O$(Fmt-Num $Step)", "1M$(Fmt-Num (-$Step))")
        # 速度环 = 被测
        Send-Cmd $scope @("2P$(Fmt-Num $Kp)", "2I$(Fmt-Num $Ki)", "2D$(Fmt-Num $Kd)",
                          '2F0', '2B0',
                          "2O$(Fmt-Num $OutMax)", "2M$(Fmt-Num (-$OutMax))")

        $hold = $st.Angle
        Send-Cmd $scope @("1T$(Fmt-Num $hold)")
        Start-Sleep -Milliseconds 500

        # 目标拉远 30 rad，保证 1.25s 采集窗口内 Angle.Out 一直饱和 = 恒速阶跃
        $trig = "1T$(Fmt-Num ($hold + 30.0))"
        $ch = 'D'; $expected = $Step
        Write-Host "内环阶跃: Omega.Target 0 -> $Step rad/s (Angle.Out 饱和), 观察 Omega.Actual" -ForegroundColor Cyan
    }
    else {
        Send-Cmd $scope @("2P$(Fmt-Num $InnerKp)", "2I$(Fmt-Num $InnerKi)", "2D$(Fmt-Num $InnerKd)",
                          '2F0', '2B0', '2O16384', '2M-16384')
        Send-Cmd $scope @("1P$(Fmt-Num $Kp)", "1I$(Fmt-Num $Ki)", "1D$(Fmt-Num $Kd)",
                          "1O$(Fmt-Num $OutMax)", "1M$(Fmt-Num (-$OutMax))")

        $hold = $st.Angle
        Send-Cmd $scope @("1T$(Fmt-Num $hold)")
        Start-Sleep -Milliseconds 500

        $trig = "1T$(Fmt-Num ($hold + $Step))"
        $ch = 'B'; $expected = $hold + $Step
        Write-Host "外环阶跃: Angle.Target $hold -> $($hold+$Step) rad, 观察 Angle.Actual" -ForegroundColor Cyan
    }

    $n = $scope.CaptureTriggered($Pre + $Post, $csv, $trig, $Pre)
    $tTrig = $scope.TriggerTime

    # 采完立刻断力，别让轮子继续飞
    Stop-Motor $scope

    $rows = Read-VofaCsv $csv
    $base = Get-ChMean $rows $ch ($tTrig - 0.15) ($tTrig - 0.02)
    $m = Get-StepMetrics -Rows $rows -Ch $ch -TStep $tTrig -Baseline $base -ExpectedFinal $expected

    # 存一份元数据：事后离线重算指标时不用再动电机
    @(
        "loop=$Loop", "ch=$ch", "Kp=$Kp", "Ki=$Ki", "Kd=$Kd", "Step=$Step",
        "OutMax=$OutMax", "InnerKp=$InnerKp", "InnerKi=$InnerKi", "InnerKd=$InnerKd",
        "tStep=$(Fmt-Num $tTrig)", "baseline=$(Fmt-Num $base)", "expected=$(Fmt-Num $expected)"
    ) | Set-Content -Path ($csv -replace '\.csv$', '.meta.txt') -Encoding UTF8

    Write-Host ("`n采集 {0} 帧  {1:N1} Hz   触发时刻 t={2:N4}s" -f $n, $scope.MeasuredRate, $tTrig)
    Write-Host ("基线={0:N5}  期望终值={1:N5}  阶跃={2:N5}" -f $m.Baseline, $m.Expected, $m.Step)

    if ($m.Valid) {
        Write-Host "`n--- 阶跃响应指标 ---" -ForegroundColor Green
        Write-Host ("  上升时间 (10-90%)  : {0}" -f $(if ($null -ne $m.RiseTime) { "{0:N2} ms" -f ($m.RiseTime*1000) } else { "未达到 90%" }))
        Write-Host ("  超调量             : {0:N1} %" -f $m.OvershootPct)
        Write-Host ("  峰值 / 峰值时刻    : {0:N5}  @ {1:N1} ms" -f $m.Peak, ($m.PeakTime*1000))
        Write-Host ("  调节时间           : {0:N1} ms   (稳态带 +-{1:N4})" -f ($m.SettleTime*1000), $m.SettleBand)
        Write-Host ("  稳态均值 / 静差    : {0:N5}  ({1:N2} %)" -f $m.SteadyMean, $m.SteadyErrPct)
        Write-Host ("  纹波 峰峰 / 标准差 : {0:N5}  /  {1:N5}" -f $m.RipplePP, $m.RippleStd)
        if ($m.Note -ne '') { Write-Host ("  注: {0}" -f $m.Note) -ForegroundColor Yellow }
    } else {
        Write-Host ("指标无效: {0}" -f $m.Note) -ForegroundColor Red
    }

    Write-Host "`n波形 (竖线 = 阶跃时刻):" -ForegroundColor Cyan
    Show-AsciiPlot $rows $ch $tTrig

    Write-Host "`n数据: $csv" -ForegroundColor Green
}
finally {
    Stop-Motor $scope
    $scope.Close()
    Write-Host "[已断力并关闭串口]" -ForegroundColor DarkGray
}
