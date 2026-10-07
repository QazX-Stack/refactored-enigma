# ============================================================
#  TuneLib.ps1 -- 阶跃测试封装 + 安全护栏 + 评分
# ============================================================
. "$PSScriptRoot\VofaLib.ps1"

# 硬性安全护栏：任何超出这些范围的值都不允许下发。
# 这些是"绝不允许越过"的线，不是"调参范围"，调参范围应该远小于它。
$script:Guard = @{
    InnerKpMax = 10000.0
    InnerKiMax = 20000.0
    InnerKdMax = 200.0
    # 外环上限从 100 提到 300：实测 Kp=100 时上升时间已锁死在限速段、
    # 静差归零，但它在 85~100 那道静摩擦阈值上只高约 10%，余量太薄。
    # 按 PM = 90 - atan(0.007*Kp) 估算 Kp=300 时 PM 仍有约 25°，
    # 而台架上真实的不稳定会先由超调表现出来（判废线 20%）。
    OuterKpMax = 300.0
    OuterKiMax = 50.0
    # pid.c:39 的 D 是裸差分不除 dt，所以等价连续微分增益 Kd_c = Kd*0.001。
    # 被控对象近似 1/s，L(s) = (Kp + Kd_c*s)/s，要存在穿越频率必须 Kd_c < 1，
    # 即 Kd < 1000 —— 这就是解析上的不稳定边界，护栏卡在这里。
    # 实际工作区在 Kd 200~700，离边界留了余量。
    OuterKdMax = 1000.0
    StepMax    = 10.0     # inner: rad/s ; outer: rad
    TorqueMax  = 16384.0
    SpeedMax   = 30.0
    # 单次实验一旦超过这些，立刻中止整轮扫描
    AbortOvershootPct = 60.0
}

function Assert-Guard {
    param([string]$Name, [double]$Value, [double]$Max)
    if ([double]::IsNaN($Value)) { throw "参数 $Name 非法 (NaN)" }
    if ([Math]::Abs($Value) -gt $Max) {
        throw "安全护栏: $Name=$Value 超过上限 $Max，拒绝下发"
    }
}

# 跑一次阶跃测试并返回指标。函数内部保证：
#   - 开始前断力并把电机停稳（保证基线干净）
#   - 采集结束立刻断力
function Invoke-StepTest {
    param(
        $Scope, [string]$LogDir,
        [ValidateSet('inner', 'outer')][string]$Loop = 'inner',
        [double]$Kp, [double]$Ki = 0, [double]$Kd = 0,
        [double]$Step = 5.0, [double]$OutMax = 16384,
        [double]$InnerKp = 2000, [double]$InnerKi = 0, [double]$InnerKd = 0,
        [string]$Tag,
        [double]$Pre = 0.30, [double]$Post = 1.00
    )

    if ($Loop -eq 'inner') {
        Assert-Guard 'InnerKp' $Kp $script:Guard.InnerKpMax
        Assert-Guard 'InnerKi' $Ki $script:Guard.InnerKiMax
        Assert-Guard 'InnerKd' $Kd $script:Guard.InnerKdMax
        Assert-Guard 'Step'    $Step $script:Guard.StepMax
        Assert-Guard 'OutMax'  $OutMax $script:Guard.TorqueMax
    } else {
        Assert-Guard 'OuterKp' $Kp $script:Guard.OuterKpMax
        Assert-Guard 'OuterKi' $Ki $script:Guard.OuterKiMax
        Assert-Guard 'OuterKd' $Kd $script:Guard.OuterKdMax
        Assert-Guard 'Step'    $Step $script:Guard.StepMax
        Assert-Guard 'OutMax'  $OutMax $script:Guard.SpeedMax
    }

    $tmpCsv = Join-Path $LogDir '_state.csv'
    $csv    = Join-Path $LogDir "$Tag.csv"

    Stop-Motor $Scope
    if (-not (Wait-MotorStop $Scope $tmpCsv 3.0)) {
        Write-Host "  [警告] 电机未停稳，基线可能不干净" -ForegroundColor Yellow
    }
    $st = Read-State $Scope $tmpCsv 0.20

    if ($Loop -eq 'inner') {
        # 角度环当"饱和发生器"：Kp=1 足以把 Angle.Out 顶到 +-Step，
        # 但对位置抖动几乎不敏感（1e-5 rad 抖动 -> 1e-5 rad/s），基线干净。
        # 目标拉远 30 rad 保证整个采集窗口内 Angle.Out 恒为 Step ——
        # 这样速度环拿到的就是一个真正的阶跃。
        Send-Cmd $Scope @('1P1', '1I0', '1D0',
                          "1O$(Fmt-Num $Step)", "1M$(Fmt-Num (-$Step))")
        Send-Cmd $Scope @("2P$(Fmt-Num $Kp)", "2I$(Fmt-Num $Ki)", "2D$(Fmt-Num $Kd)",
                          '2F0', '2B0',
                          "2O$(Fmt-Num $OutMax)", "2M$(Fmt-Num (-$OutMax))")
        $hold = $st.Angle
        Send-Cmd $Scope @("1T$(Fmt-Num $hold)")
        Start-Sleep -Milliseconds 500
        $trig = "1T$(Fmt-Num ($hold + 30.0))"
        $ch = 'D'; $expected = $Step
    }
    else {
        Send-Cmd $Scope @("2P$(Fmt-Num $InnerKp)", "2I$(Fmt-Num $InnerKi)", "2D$(Fmt-Num $InnerKd)",
                          '2F0', '2B0', '2O16384', '2M-16384')
        # 1F0/1B0 必须显式下发：固件没有参数回读，任何没被显式写过的量
        # 都只能靠"从没人设过它"来保证是 0 —— 那不是保证。
        Send-Cmd $Scope @("1P$(Fmt-Num $Kp)", "1I$(Fmt-Num $Ki)", "1D$(Fmt-Num $Kd)",
                          '1F0', '1B0',
                          "1O$(Fmt-Num $OutMax)", "1M$(Fmt-Num (-$OutMax))")
        $hold = $st.Angle
        Send-Cmd $Scope @("1T$(Fmt-Num $hold)")
        Start-Sleep -Milliseconds 500
        $trig = "1T$(Fmt-Num ($hold + $Step))"
        $ch = 'B'; $expected = $hold + $Step
    }

    $n = $Scope.CaptureTriggered($Pre + $Post, $csv, $trig, $Pre)
    $tTrig = $Scope.TriggerTime
    Stop-Motor $Scope        # 采完立刻断力，绝不让轮子带着最后那组参数继续飞

    $rows = Read-VofaCsv $csv
    $base = Get-ChMean $rows $ch ($tTrig - 0.15) ($tTrig - 0.02)
    $m = Get-StepMetrics -Rows $rows -Ch $ch -TStep $tTrig -Baseline $base -ExpectedFinal $expected

    @(
        "loop=$Loop", "ch=$ch", "Kp=$Kp", "Ki=$Ki", "Kd=$Kd", "Step=$Step", "OutMax=$OutMax",
        "InnerKp=$InnerKp", "InnerKi=$InnerKi", "InnerKd=$InnerKd",
        "tStep=$(Fmt-Num $tTrig)", "baseline=$(Fmt-Num $base)", "expected=$(Fmt-Num $expected)"
    ) | Set-Content -Path ($csv -replace '\.csv$', '.meta.txt') -Encoding UTF8

    return [pscustomobject]@{
        Tag = $Tag; Loop = $Loop; Ch = $ch; Kp = $Kp; Ki = $Ki; Kd = $Kd; Step = $Step
        Frames = $n; Rate = $Scope.MeasuredRate
        Metrics = $m; Rows = $rows; Csv = $csv
    }
}

# 内环评分：稳定性(纹波)权重最高 —— 内环振荡会直接毁掉外环的可调性；
# 其次上升时间（决定外环能做到多快）；超调对内环最不重要（外环能补）。
# 归一化后再加权，避免单位不同的量纲互相压制。
function Get-InnerCost {
    param($M)
    if (-not $M.Valid) { return [double]::PositiveInfinity }
    if ($M.OvershootPct -gt 25.0) { return [double]::PositiveInfinity }
    if ($null -eq $M.RiseTime) { return [double]::PositiveInfinity }
    $relRipple = $M.RippleStd / [Math]::Abs($M.Step)
    $riseMs    = $M.RiseTime * 1000.0
    return 50.0 * $relRipple + ($riseMs / 10.0) + 0.1 * $M.OvershootPct
}

# 外环评分：位置超调是用户直接能感觉到的东西，权重最高，超过 20% 直接判废。
# 其次是上升时间；位置纹波也要压制 —— 外环 Kp 越低，速度环底噪被积分成的
# 位置抖动越大（σ_pos ≈ σ_vel/Kp），所以这一项实际是在鼓励提高 Kp。
function Get-OuterCost {
    param($M)
    if (-not $M.Valid) { return [double]::PositiveInfinity }
    if ($null -eq $M.RiseTime) { return [double]::PositiveInfinity }
    if ($M.OvershootPct -gt 20.0) { return [double]::PositiveInfinity }
    $relRipple = $M.RippleStd / [Math]::Abs($M.Step)
    return 1.0 * $M.OvershootPct + ($M.RiseTime * 1000.0) / 5.0 + 100.0 * $relRipple
}

function Get-Cost {
    param($M, [string]$Loop = 'inner')
    if ($Loop -eq 'outer') { return Get-OuterCost $M }
    return Get-InnerCost $M
}

function Write-MetricTable {
    param($Results, [string]$Loop = 'inner')
    Write-Host ("{0,-22} {1,9} {2,9} {3,9} {4,9} {5,9} {6,8}" -f `
        'Tag','上升ms','超调%','静差%','纹波σ','纹波PP','代价')
    Write-Host ('-' * 80)
    foreach ($r in $Results) {
        $m = $r.Metrics
        $cost = Get-Cost $m $Loop
        $costStr = if ([double]::IsPositiveInfinity($cost)) { '  废' } else { "{0,8:N2}" -f $cost }
        $riseStr = if ($null -ne $m.RiseTime) { "{0,9:N2}" -f ($m.RiseTime*1000) } else { '        -' }
        Write-Host ("{0,-22} {1} {2,9:N2} {3,9:N2} {4,9:N4} {5,9:N4} {6}" -f `
            $r.Tag, $riseStr, $m.OvershootPct, $m.SteadyErrPct, $m.RippleStd, $m.RipplePP, $costStr)
    }
}
