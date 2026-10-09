param([Parameter(Mandatory=$true)][string]$Root,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$sourceRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\')
$previewRoot = [IO.Path]::GetFullPath($Output)
$wallpapers = Join-Path $sourceRoot 'Assets/wallpaper'
$jpegCodec = [Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }
foreach ($wallpaper in Get-ChildItem -LiteralPath $wallpapers -File -Filter '*.jpg' -Recurse) {
    $relativePath = $wallpaper.FullName.Substring($sourceRoot.Length + 1)
    $targetPath = Join-Path $previewRoot $relativePath
    $stampPath = $targetPath + '.stamp'
    $stamp = 'native-wallpaper-1920-v1|' + $wallpaper.LastWriteTimeUtc.Ticks + '|' + $wallpaper.Length
    if ((Test-Path -LiteralPath $targetPath) -and (Test-Path -LiteralPath $stampPath) -and ([IO.File]::ReadAllText($stampPath) -eq $stamp)) { continue }
    $image = [Drawing.Image]::FromFile($wallpaper.FullName)
    try {
        if ($image.Width -le 1920 -and $image.Height -le 1920) { continue }
        $scale = 1920.0 / [Math]::Max($image.Width,$image.Height)
        $width = [Math]::Max(1,[int][Math]::Round($image.Width * $scale))
        $height = [Math]::Max(1,[int][Math]::Round($image.Height * $scale))
        $bitmap = New-Object Drawing.Bitmap($width,$height)
        try {
            $graphics = [Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($image,0,0,$width,$height)
            } finally { $graphics.Dispose() }
            [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($targetPath)) | Out-Null
            $parameters = New-Object Drawing.Imaging.EncoderParameters(1)
            try {
                $parameters.Param[0] = New-Object Drawing.Imaging.EncoderParameter([Drawing.Imaging.Encoder]::Quality,[long]90)
                $bitmap.Save($targetPath,$jpegCodec,$parameters)
            } finally { $parameters.Dispose() }
            [IO.File]::WriteAllText($stampPath,$stamp)
            Write-Output ('Native wallpaper: {0} ({1}x{2})' -f $relativePath,$width,$height)
        } finally { $bitmap.Dispose() }
    } finally { $image.Dispose() }
}
