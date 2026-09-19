<#
  阶段 1：速度环 Kp 扫描（Ki=Kd=0）

  两条安全阀：
    - 单次超调 > 25%  -> 判废，不参与评分
    - 单次超调 > 60%  -> 立刻中止整轮扫描（说明已经在不稳定边缘，
                         继续往上加只会打出更剧烈的振荡）
#>
param(
    [double[]]$KpList = @(400, 800, 1500, 2000, 3000, 4500, 6500),
    [double]$Step     = 5.0,
    [double]$OutMax   = 16384,
    [string]$Prefix   = 'inner_kp',
    [string]$Port     = 'COM7',
    [int]$Baud        = 460800
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\TuneLib.ps1"

$LogDir = Join-Path $PSScriptRoot 'logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir | Out-Null }

$scope = New-Object VofaScope
$scope.Open($Port, $Baud)

$results = New-Object 'System.Collections.Generic.List[object]'
$aborted = $false

try {
    Write-Host "速度环 Kp 扫描  (阶跃 $Step rad/s, 力矩限幅 $OutMax)" -ForegroundColor Cyan
    Write-Host "共 $($KpList.Count) 组，每组约 3 秒`n"

    $i = 0
    foreach ($kp in $KpList) {
        $i++
        Write-Host ("[{0}/{1}] Kp={2}" -f $i, $KpList.Count, $kp) -ForegroundColor White

        # 序号进 tag：允许同一个 Kp 重复测多次来量化重复性
        $tag = "{0}_{1}_r{2}" -f $Prefix, $kp, $i
        $r = Invoke-StepTest -Scope $scope -LogDir $LogDir -Loop inner `
                             -Kp $kp -Ki 0 -Kd 0 -Step $Step -OutMax $OutMax -Tag $tag
        $results.Add($r)

        $m = $r.Metrics
        if ($m.Valid) {
            Write-Host ("        上升 {0:N2}ms  超调 {1:N2}%  静差 {2:N2}%  纹波σ {3:N4}" -f `
                ($m.RiseTime*1000), $m.OvershootPct, $m.SteadyErrPct, $m.RippleStd)
        } else {
            Write-Host ("        指标无效: {0}" -f $m.Note) -ForegroundColor Red
        }

        if ($m.Valid -and $m.OvershootPct -gt $script:Guard.AbortOvershootPct) {
            Write-Host ("`n!!! 超调 {0:N1}% 超过中止阈值 {1}% —— 立刻停止扫描" -f `
                $m.OvershootPct, $script:Guard.AbortOvershootPct) -ForegroundColor Red
            $aborted = $true
            break
        }

        Start-Sleep -Milliseconds 400   # 让驱动器和电机喘口气
    }

    Write-Host "`n===== 扫描结果 =====" -ForegroundColor Cyan
    Write-MetricTable $results

    if ($aborted) { Write-Host "`n[扫描被安全阀中止，结果不完整]" -ForegroundColor Red }

    $best = $results | Where-Object { -not [double]::IsPositiveInfinity((Get-InnerCost $_.Metrics)) } |
                      Sort-Object { Get-InnerCost $_.Metrics } | Select-Object -First 1
    if ($best) {
        Write-Host ("`n推荐 Kp = {0}   代价 {1:N2}" -f $best.Kp, (Get-InnerCost $best.Metrics)) -ForegroundColor Green
    } else {
        Write-Host "`n没有一组通过约束" -ForegroundColor Red
    }
}
finally {
    Stop-Motor $scope
    $scope.Close()
    Write-Host "[已断力并关闭串口]" -ForegroundColor DarkGray
}
