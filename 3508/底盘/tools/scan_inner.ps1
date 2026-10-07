<#
  速度环参数扫描。用例写成 'Kp,Ki,Kd' 字符串，方便一次扫任意组合。

  安全阀：
    - 单次超调 > 25% -> 判废
    - 单次超调 > 60% -> 立刻中止（已经在不稳定边缘，继续只会更剧烈）
#>
param(
    [string[]]$Cases = @('2500,0,0'),
    [ValidateSet('inner', 'outer')][string]$Loop = 'inner',
    [double]$Step   = 5.0,
    [double]$OutMax = 0,          # 0 = 按 Loop 取默认（内环 16384 力矩 / 外环 25 rad/s 限速）
    [double]$InnerKp = 2500,      # 仅外环扫描时生效：固定住已定案的内环
    [double]$InnerKi = 4000,
    [double]$InnerKd = 0,
    [string]$Prefix = 'inner',
    [double]$Pre    = 0.30,
    [double]$Post   = 1.00,
    [string]$Port   = 'COM7',
    [int]$Baud      = 460800
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\TuneLib.ps1"

if ($OutMax -le 0) { $OutMax = if ($Loop -eq 'outer') { 25.0 } else { 16384.0 } }

$LogDir = Join-Path $PSScriptRoot 'logs'
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir | Out-Null }

$scope = New-Object VofaScope
$scope.Open($Port, $Baud)

$results = New-Object 'System.Collections.Generic.List[object]'
$aborted = $false

try {
    if ($Loop -eq 'outer') {
        Write-Host "角度环扫描  阶跃 $Step rad  限速 $OutMax rad/s  采集 $($Pre+$Post)s" -ForegroundColor Cyan
        Write-Host "内环固定为 Kp=$InnerKp Ki=$InnerKi Kd=$InnerKd" -ForegroundColor DarkGray
    } else {
        Write-Host "速度环扫描  阶跃 $Step rad/s  力矩限幅 $OutMax  采集 $($Pre+$Post)s" -ForegroundColor Cyan
    }
    Write-Host "共 $($Cases.Count) 组`n"

    $i = 0
    foreach ($case in $Cases) {
        $i++
        $p = $case.Split(',')
        if ($p.Count -lt 3) {
            throw "用例 '$case' 格式不对：应为 'Kp,Ki,Kd' 字符串。注意 PowerShell 会把 -Cases 2500,0,0 拆成数组，必须写成 -Cases '2500,0,0'"
        }
        $kp = [double]::Parse($p[0], [Globalization.CultureInfo]::InvariantCulture)
        $ki = [double]::Parse($p[1], [Globalization.CultureInfo]::InvariantCulture)
        $kd = [double]::Parse($p[2], [Globalization.CultureInfo]::InvariantCulture)

        if ($ki -eq 0 -and $kd -eq 0) { $tag = "{0}_kp{1}_r{2}"      -f $Prefix, $kp, $i }
        else                          { $tag = "{0}_kp{1}_ki{2}_kd{3}_r{4}" -f $Prefix, $kp, $ki, $kd, $i }

        Write-Host ("[{0}/{1}] Kp={2} Ki={3} Kd={4}" -f $i, $Cases.Count, $kp, $ki, $kd) -ForegroundColor White

        $r = Invoke-StepTest -Scope $scope -LogDir $LogDir -Loop $Loop `
                             -Kp $kp -Ki $ki -Kd $kd -Step $Step -OutMax $OutMax `
                             -InnerKp $InnerKp -InnerKi $InnerKi -InnerKd $InnerKd `
                             -Tag $tag -Pre $Pre -Post $Post
        $results.Add($r)

        $m = $r.Metrics
        if ($m.Valid) {
            Write-Host ("        上升 {0:N2}ms  超调 {1:N2}%  静差 {2:N2}%  调节 {3:N0}ms  纹波σ {4:N4}" -f `
                ($m.RiseTime*1000), $m.OvershootPct, $m.SteadyErrPct, ($m.SettleTime*1000), $m.RippleStd)
        } else {
            Write-Host ("        指标无效: {0}" -f $m.Note) -ForegroundColor Red
        }

        if ($m.Valid -and $m.OvershootPct -gt $script:Guard.AbortOvershootPct) {
            Write-Host ("`n!!! 超调 {0:N1}% 超过中止阈值 {1}% —— 立刻停止扫描" -f `
                $m.OvershootPct, $script:Guard.AbortOvershootPct) -ForegroundColor Red
            $aborted = $true
            break
        }

        Start-Sleep -Milliseconds 400
    }

    Write-Host "`n===== 扫描结果 =====" -ForegroundColor Cyan
    Write-MetricTable $results -Loop $Loop

    $best = $results | Where-Object { -not [double]::IsPositiveInfinity((Get-Cost $_.Metrics $Loop)) } |
                      Sort-Object { Get-Cost $_.Metrics $Loop } | Select-Object -First 1
    if ($best) {
        Write-Host ("`n当前口径下最优: Kp={0} Ki={1} Kd={2}  代价 {3:N2}" -f `
            $best.Kp, $best.Ki, $best.Kd, (Get-Cost $best.Metrics $Loop)) -ForegroundColor Green
    } else {
        Write-Host "`n没有一组通过约束" -ForegroundColor Red
    }

    if ($aborted) { Write-Host "`n[扫描被安全阀中止，结果不完整]" -ForegroundColor Red }
}
finally {
    Stop-Motor $scope
    $scope.Close()
    Write-Host "[已断力并关闭串口]" -ForegroundColor DarkGray
}
