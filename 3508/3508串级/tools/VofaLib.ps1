# ============================================================
#  VofaLib.ps1  --  VOFA+ JustFloat 串口采集与阶跃响应分析
#  依赖：仅 .NET System.IO.Ports，无需安装 Python/任何包
#  协议：4 x float32 小端 + 帧尾 00 00 80 7F，共 20 字节/帧
#  信道：A=Angle.Target  B=Angle.Actual  C=Omega.Target  D=Omega.Actual
# ============================================================

if (-not ("VofaScope" -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Text;

public class VofaScope : IDisposable
{
    private SerialPort sp;
    private Stopwatch sw;
    private readonly byte[] buf = new byte[20];
    private int filled = 0;

    public long BadFrames;
    public long GoodFrames;
    public double MeasuredRate;   // 实测帧率 (n-1)/(tLast-tFirst)

    public void Open(string portName, int baud)
    {
        if (sp != null && sp.IsOpen) sp.Close();
        sp = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
        sp.ReadTimeout = 400;
        sp.WriteTimeout = 400;
        sp.Open();
        sw = Stopwatch.StartNew();
        filled = 0; BadFrames = 0; GoodFrames = 0;
    }

    public void Close()
    {
        if (sp != null && sp.IsOpen) sp.Close();
    }

    public void Dispose() { Close(); }

    public bool IsOpen { get { return sp != null && sp.IsOpen; } }

    public void Send(string cmd)
    {
        if (sp == null || !sp.IsOpen) throw new InvalidOperationException("port not open");
        sp.Write(cmd + "\n");
    }

    private bool ReadFrame(out double t, out float[] v)
    {
        t = 0; v = null;
        while (filled < 20)
        {
            int n;
            try { n = sp.Read(buf, filled, 20 - filled); }
            catch (TimeoutException) { filled = 0; return false; }
            if (n <= 0) { filled = 0; return false; }
            filled += n;
        }
        if (buf[16] != 0x00 || buf[17] != 0x00 || buf[18] != 0x80 || buf[19] != 0x7F)
        {
            // 帧尾对不上 -> 左移一字节重新同步
            Buffer.BlockCopy(buf, 1, buf, 0, 19);
            filled = 19;
            BadFrames++;
            return false;
        }
        float a = BitConverter.ToSingle(buf, 0);
        float b = BitConverter.ToSingle(buf, 4);
        float c = BitConverter.ToSingle(buf, 8);
        float d = BitConverter.ToSingle(buf, 12);
        if (float.IsNaN(a) || float.IsInfinity(a) ||
            float.IsNaN(b) || float.IsInfinity(b) ||
            float.IsNaN(c) || float.IsInfinity(c) ||
            float.IsNaN(d) || float.IsInfinity(d))
        {
            filled = 0; BadFrames++;
            return false;
        }
        v = new float[] { a, b, c, d };
        t = sw.Elapsed.TotalSeconds;
        filled = 0;
        GoodFrames++;
        return true;
    }

    public double TriggerTime;    // 触发命令发出的时刻（秒，同 CSV 的 t 轴）

    public int Capture(double seconds, string csvPath)
    {
        return Run(seconds, csvPath, null, 0.0);
    }

    /// 采集 seconds 秒；在 t=triggerAt 时由采集程序自己发出 triggerCmd。
    /// 阶跃必须这样发——交给 PowerShell 调度会引入几毫秒不定的延迟，
    /// 直接毁掉上升时间的测量。
    public int CaptureTriggered(double seconds, string csvPath, string triggerCmd, double triggerAt)
    {
        return Run(seconds, csvPath, triggerCmd, triggerAt);
    }

    private int Run(double seconds, string csvPath, string triggerCmd, double triggerAt)
    {
        var sb = new StringBuilder(1 << 21);
        sb.Append("t,n,A,B,C,D\n");
        var ci = CultureInfo.InvariantCulture;

        // 丢弃驱动缓冲里的陈旧字节并把秒表归零，否则缓冲积压会让
        // "帧数/时长"算出 >100% 的假帧率（实测会虚高到 1017Hz）
        try { sp.DiscardInBuffer(); } catch { }
        filled = 0;
        sw.Restart();

        int n = 0;
        double t = 0, tFirst = 0, tLast = 0;
        bool fired = (triggerCmd == null);
        TriggerTime = double.NaN;
        float[] v;

        while (sw.Elapsed.TotalSeconds < seconds)
        {
            double now = sw.Elapsed.TotalSeconds;
            if (!fired && now >= triggerAt)
            {
                Send(triggerCmd);
                TriggerTime = now;   // 用发送时刻作为阶跃时刻
                fired = true;
            }
            if (!ReadFrame(out t, out v)) continue;
            if (n == 0) { tFirst = t; }
            tLast = t;
            sb.Append(t.ToString("F5", ci)).Append(',').Append(n).Append(',')
              .Append(v[0].ToString("R", ci)).Append(',')
              .Append(v[1].ToString("R", ci)).Append(',')
              .Append(v[2].ToString("R", ci)).Append(',')
              .Append(v[3].ToString("R", ci)).Append('\n');
            n++;
        }
        // n 帧只跨越 n-1 个间隔
        MeasuredRate = (n > 1 && tLast > tFirst) ? (n - 1) / (tLast - tFirst) : 0.0;
        File.WriteAllText(csvPath, sb.ToString());
        return n;
    }
}
'@
}

# ------------------------------------------------------------
#  以下分析全部用 PowerShell 写，方便随时改口径而不用重编译
# ------------------------------------------------------------

function Read-VofaCsv {
    param([string]$Path)
    $ci = [Globalization.CultureInfo]::InvariantCulture
    $lines = [System.IO.File]::ReadAllLines($Path)
    $out = New-Object 'System.Collections.Generic.List[object]'
    for ($i = 1; $i -lt $lines.Length; $i++) {
        if ($lines[$i].Length -eq 0) { continue }
        $p = $lines[$i].Split(',')
        $out.Add([pscustomobject]@{
            t = [double]::Parse($p[0], $ci)
            n = [int]::Parse($p[1], $ci)
            A = [double]::Parse($p[2], $ci)
            B = [double]::Parse($p[3], $ci)
            C = [double]::Parse($p[4], $ci)
            D = [double]::Parse($p[5], $ci)
        })
    }
    return $out
}

function Get-ChannelValues {
    param($Rows, [string]$Ch)
    $vals = New-Object 'System.Collections.Generic.List[double]'
    foreach ($r in $Rows) { $vals.Add($r.$Ch) }
    return $vals
}

function Get-Mean {
    param($Values, [double]$FromT, [double]$ToT, $Times)
    $s = 0.0; $n = 0
    for ($i = 0; $i -lt $Values.Count; $i++) {
        if ($Times[$i] -ge $FromT -and $Times[$i] -le $ToT) { $s += $Values[$i]; $n++ }
    }
    if ($n -eq 0) { return [double]::NaN }
    return $s / $n
}

# 阶跃响应指标
#
# 所有动态指标（上升 / 超调 / 调节时间）都在"滑动平均后的趋势"上计算。
# 原因：原始信号叠着约 2% 的纹波，直接取全局最大值会把纹波尖峰算成超调，
# 于是低增益组凭空多出几个百分点、高增益组的超调被高估 —— 扫描出来的
# 上升时间/超调曲线就失去判别力了。纹波本身单独作为稳定性指标报告。
function Get-StepMetrics {
    param(
        $Rows,
        [string]$Ch,
        [double]$TStep,           # 阶跃发生时刻（秒，同 CSV 时间轴）
        [double]$Baseline,        # 阶跃前稳态值
        [double]$ExpectedFinal,   # 期望终值（= 我下发的目标值）
        [double]$SmoothMs   = 5.0,   # 滑动平均窗口
        [double]$SettleBand = 0.02,
        [double]$LateFrac   = 0.25   # 取末段算稳态
    )

    $t  = Get-ChannelValues $Rows 't'
    $y  = Get-ChannelValues $Rows $Ch
    $n0 = $t.Count

    $res = [pscustomobject]@{
        Ch=$Ch; N=0; Valid=$false; Note=''
        Baseline=$Baseline; Expected=$ExpectedFinal; Step=$null
        Peak=$null; PeakTime=$null; RiseTime=$null; SettleTime=$null; SettleBand=$null
        OvershootPct=$null; RipplePP=$null; RippleStd=$null
        SteadyMean=$null; SteadyErrPct=$null; Vpp=$null
    }

    if ($n0 -lt 40) { $res.Note = '数据太少'; return $res }

    $dt = ($t[$n0-1] - $t[0]) / ($n0 - 1)
    if ($dt -le 0) { $res.Note = '时间轴异常'; return $res }

    # ---- 居中滑动平均（窗口在边界自动收窄） ----
    $k    = [Math]::Max(1, [int][Math]::Round(($SmoothMs/1000.0) / $dt))
    $half = [int][Math]::Floor($k/2)
    $ys   = New-Object 'double[]' $n0
    for ($i = 0; $i -lt $n0; $i++) {
        $a = [Math]::Max(0, $i-$half); $b = [Math]::Min($n0-1, $i+$half)
        $s = 0.0
        for ($j = $a; $j -le $b; $j++) { $s += $y[$j] }
        $ys[$i] = $s / ($b - $a + 1)
    }

    $post = New-Object 'System.Collections.Generic.List[int]'
    for ($i = 0; $i -lt $n0; $i++) { if ($t[$i] -ge $TStep) { $post.Add($i) } }
    $res.N = $post.Count
    if ($post.Count -lt 30) { $res.Note = '阶跃后数据太少'; return $res }

    $step = $ExpectedFinal - $Baseline
    $res.Step = $step
    if ([Math]::Abs($step) -lt 1e-9) { $res.Note = '阶跃幅度为 0'; return $res }
    $dir = [Math]::Sign($step)

    # ---- 末段：趋势上取稳态均值；原始去趋势的残差上取纹波 ----
    # $post 存的是"帧下标"，遍历用的是"在 $post 里的位置"，两者不能混用
    $p0 = [int]($post.Count * (1.0 - $LateFrac))
    $s = 0.0; $n = 0
    for ($p = $p0; $p -lt $post.Count; $p++) { $s += $ys[$post[$p]]; $n++ }
    $steady = $s / $n
    $res.SteadyMean   = $steady
    $res.SteadyErrPct = ($steady - $ExpectedFinal) / $step * 100.0

    # 纹波必须用固定的趋势窗来定义，不能跟着 $SmoothMs 走 ——
    # 否则 RippleStd 实际测的是"这个窗滤掉了多少"，换个窗就换个数，
    # 跨实验之间就没法比较了。
    $kr = [Math]::Max(1, [int][Math]::Round(0.010 / $dt))
    $hr = [int][Math]::Floor($kr/2)
    $yr = New-Object 'double[]' $n0
    for ($i = 0; $i -lt $n0; $i++) {
        $a = [Math]::Max(0, $i-$hr); $b = [Math]::Min($n0-1, $i+$hr)
        $s2 = 0.0
        for ($j = $a; $j -le $b; $j++) { $s2 += $y[$j] }
        $yr[$i] = $s2 / ($b - $a + 1)
    }

    $rs = 0.0; $rs2 = 0.0; $mn = [double]::MaxValue; $mx = [double]::MinValue
    for ($p = $p0; $p -lt $post.Count; $p++) {
        $d = $y[$post[$p]] - $yr[$post[$p]]
        $rs += $d; $rs2 += $d*$d
        if ($d -lt $mn) { $mn = $d }
        if ($d -gt $mx) { $mx = $d }
    }
    $rm = $rs / $n
    $res.RippleStd = [Math]::Sqrt([Math]::Max(0.0, $rs2/$n - $rm*$rm))
    $res.RipplePP  = $mx - $mn

    # ---- 峰值/超调：第一次越过目标后 200ms 内的极值 ----
    # 只取"第一次"，不取全局最大 —— 更远的极值属于纹波或二次振荡，
    # 把它算成超调会让高增益组的数字虚高。
    $crossPos = -1
    for ($p = 0; $p -lt $post.Count; $p++) {
        if ($dir*($ys[$post[$p]] - $ExpectedFinal) -ge 0) { $crossPos = $p; break }
    }
    if ($crossPos -lt 0) {
        # 从没越过目标（静差把曲线压在目标之下），超调定义为 0
        $peak = $ys[$post[0]]; $peakI = $post[0]
        foreach ($i in $post) {
            if ($dir*($ys[$i] - $peak) -gt 0) { $peak = $ys[$i]; $peakI = $i }
        }
        $res.OvershootPct = 0.0
        $res.Note = '未越过目标（稳态静差）'
    } else {
        $pEnd = [Math]::Min($post.Count-1, $crossPos + [int](0.200/$dt))
        $peak = $ys[$post[$crossPos]]; $peakI = $post[$crossPos]
        for ($p = $crossPos; $p -le $pEnd; $p++) {
            $i = $post[$p]
            if ($dir*($ys[$i] - $peak) -gt 0) { $peak = $ys[$i]; $peakI = $i }
        }
        $res.OvershootPct = [Math]::Max(0.0, $dir*($peak - $ExpectedFinal) / [Math]::Abs($step) * 100.0)
    }
    $res.Peak     = $peak
    $res.PeakTime = $t[$peakI] - $TStep

    # ---- 峰值处的绝对值范围（判断有没有饱和） ----
    $amn = [double]::MaxValue; $amx = [double]::MinValue
    foreach ($i in $post) {
        if ($y[$i] -lt $amn) { $amn = $y[$i] }
        if ($y[$i] -gt $amx) { $amx = $y[$i] }
    }
    $res.Vpp = "{0:N3}..{1:N3}" -f $amn, $amx

    # ---- 上升时间 10% -> 90% ----
    $th10 = $Baseline + 0.1 * $step
    $th90 = $Baseline + 0.9 * $step
    $t10 = $null; $t90 = $null
    foreach ($i in $post) {
        if ($null -eq $t10 -and $dir*($ys[$i] - $th10) -ge 0) { $t10 = $t[$i] }
        if ($null -eq $t90 -and $dir*($ys[$i] - $th90) -ge 0) { $t90 = $t[$i] }
    }
    if ($null -ne $t10 -and $null -ne $t90) { $res.RiseTime = $t90 - $t10 }

    # ---- 调节时间 ----
    # 带心取"实际稳态均值"而不是"命令目标值" —— 这样 P 控制的静差会
    # 单独体现在 SteadyErrPct 里，而不会让调节时间变成永远不收敛。
    # 带宽下限 2%*|step|；若纹波更大则放宽到 2σ。
    $band = [Math]::Max($SettleBand * [Math]::Abs($step), 2.0 * $res.RippleStd)
    $res.SettleBand = $band
    $lastOut = $null
    foreach ($i in $post) {
        if ([Math]::Abs($ys[$i] - $steady) -gt $band) { $lastOut = $t[$i] }
    }
    $res.SettleTime = if ($null -ne $lastOut) { $lastOut - $TStep } else { 0.0 }

    $res.Valid = $true
    return $res
}

# 极简 ASCII 波形，便于在终端直接看
function Show-AsciiPlot {
    param($Rows, [string]$Ch, [double]$TStep = -1, [int]$Width = 78, [int]$Height = 11)
    $t = Get-ChannelValues $Rows 't'
    $y = Get-ChannelValues $Rows $Ch
    if ($t.Count -lt 2) { return }
    $mn = ($y | Measure-Object -Minimum).Minimum
    $mx = ($y | Measure-Object -Maximum).Maximum
    if ($mx - $mn -lt 1e-9) { $mx = $mn + 1e-9 }
    $grid = @()
    for ($r = 0; $r -lt $Height; $r++) { $grid += ,(New-Object char[] $Width) }
    for ($r = 0; $r -lt $Height; $r++) { for ($c = 0; $c -lt $Width; $c++) { $grid[$r][$c] = ' ' } }
    $t0 = $t[0]; $t1 = $t[$t.Count-1]
    if ($t1 - $t0 -lt 1e-9) { return }
    for ($i = 0; $i -lt $t.Count; $i++) {
        $c = [int](($t[$i] - $t0) / ($t1 - $t0) * ($Width - 1))
        $r = [int](($mx - $y[$i]) / ($mx - $mn) * ($Height - 1))
        if ($c -ge 0 -and $c -lt $Width -and $r -ge 0 -and $r -lt $Height) { $grid[$r][$c] = '*' }
    }
    if ($TStep -ge $t0 -and $TStep -le $t1) {
        $c = [int](($TStep - $t0) / ($t1 - $t0) * ($Width - 1))
        for ($r = 0; $r -lt $Height; $r++) { $grid[$r][$c] = '|' }
    }
    Write-Host ("  {0}  [{1:N3} .. {2:N3}]  span={3:N3}" -f $Ch, $mn, $mx, ($mx - $mn))
    for ($r = 0; $r -lt $Height; $r++) { Write-Host ("  " + (-join $grid[$r])) }
    Write-Host ("  t: {0:N3}s -> {1:N3}s" -f $t0, $t1)
}

# ============================================================
#  命令下发与安全原语
# ============================================================

# 数字格式化：必须用不变文化且禁用科学计数法，
# 否则 atof 在 &cmd[2] 上可能解析出意料之外的值
function Fmt-Num {
    param([double]$v)
    return $v.ToString('0.########', [Globalization.CultureInfo]::InvariantCulture)
}

# 固件 UART_Tuning_Callback 每个空闲中断只解析第一行，
# 连发多条会被整段丢弃 —— 必须逐条发、留间隔
function Send-Cmd {
    param($Scope, [string[]]$Cmds, [int]$DelayMs = 30)
    foreach ($c in $Cmds) {
        if ($c -eq $null -or $c -eq '') { continue }
        $Scope.Send($c)
        Start-Sleep -Milliseconds $DelayMs
    }
}

# 硬急停。注意 X 只清 Kp/Ki/Kd/Kf，清不掉 OutOffset；
# 而 pid.c:45 是 `if (Actual > 0.5) Out += OutOffset` —— 凭增益清零
# 并不能保证 Out=0。所以这里直接把两个环的上下限都压成 0，
# 让 pid.c:48-55 的钳位强制 Out ≡ 0，Output 必然为 0。
function Stop-Motor {
    param($Scope)
    Send-Cmd $Scope @('X', '2B0', '1O0', '1M0', '2O0', '2M0') 25
}

function Get-ChMean {
    param($Rows, [string]$Ch, [double]$FromT, [double]$ToT)
    $t = Get-ChannelValues $Rows 't'
    $v = Get-ChannelValues $Rows $Ch
    return Get-Mean -Values $v -FromT $FromT -ToT $ToT -Times $t
}

function Get-ChLast {
    param($Rows, [string]$Ch)
    return $Rows[$Rows.Count-1].$Ch
}

# 短采一段，读回当前状态（位置 / 转速）
function Read-State {
    param($Scope, [string]$Path, [double]$Seconds = 0.15)
    $null = $Scope.Capture($Seconds, $Path)
    $rows = Read-VofaCsv $Path
    return [pscustomobject]@{
        Angle   = Get-ChMean $rows 'B' $rows[0].t $rows[$rows.Count-1].t
        Omega   = Get-ChMean $rows 'D' $rows[0].t $rows[$rows.Count-1].t
        OmegaPP = ((Get-ChannelValues $rows 'D' | Measure-Object -Maximum).Maximum -
                   (Get-ChannelValues $rows 'D' | Measure-Object -Minimum).Minimum)
        Rows    = $rows
    }
}

# 等电机停稳（断力后靠摩擦自然减速）
function Wait-MotorStop {
    param($Scope, [string]$TmpPath, [double]$TimeoutSec = 3.0, [double]$Thresh = 0.15)
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
        $st = Read-State $Scope $TmpPath 0.12
        if ([Math]::Abs($st.Omega) -lt $Thresh) { return $true }
    }
    return $false
}
