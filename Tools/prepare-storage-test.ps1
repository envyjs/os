param([string]$Output='Build/Tests/Storage/fat32.img',[switch]$Superfloppy)
$ErrorActionPreference='Stop'
$target=[IO.Path]::GetFullPath($Output)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
$start=2048
if($Superfloppy){$start=0}
$total=131072
$stream=[IO.File]::Open($target,[IO.FileMode]::Create,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
function Put16($b,$offset,$value){[Array]::Copy([BitConverter]::GetBytes([uint16]$value),0,$b,$offset,2)}
function Put32($b,$offset,$value){[Array]::Copy([BitConverter]::GetBytes([uint32]$value),0,$b,$offset,4)}
function Text($b,$offset,$value){$bytes=[Text.Encoding]::ASCII.GetBytes($value);[Array]::Copy($bytes,0,$b,$offset,$bytes.Length)}
function Sector($lba,$b){$stream.Position=[int64]$lba*512;$stream.Write($b,0,$b.Length)}
function Entry($b,$offset,$name,$cluster,$size,$attributes=32){Text $b $offset $name;$b[$offset+11]=$attributes;Put16 $b ($offset+26) $cluster;Put32 $b ($offset+28) $size}
try {
    $stream.SetLength(([int64]$start+$total)*512)
    if($start){$mbr=New-Object byte[] 512;$mbr[446]=128;$mbr[450]=12;Put32 $mbr 454 $start;Put32 $mbr 458 $total;Put16 $mbr 510 43605;Sector 0 $mbr}
    $boot=New-Object byte[] 512;$boot[0]=235;$boot[1]=88;$boot[2]=144;Text $boot 3 'ENVYFAT ';Put16 $boot 11 512;$boot[13]=1;Put16 $boot 14 32;$boot[16]=2;$boot[21]=248;Put32 $boot 28 $start;Put32 $boot 32 $total;Put32 $boot 36 1024;Put32 $boot 44 2;Put16 $boot 48 1;Put16 $boot 50 6;$boot[64]=128;$boot[66]=41;Text $boot 71 'ENVY TEST  ';Text $boot 82 'FAT32   ';Put16 $boot 510 43605;Sector $start $boot;Sector ($start+6) $boot
    $info=New-Object byte[] 512;Put32 $info 0 1096897106;Put32 $info 484 1631679090;Put32 $info 488 4294967295;Put32 $info 492 8;Put32 $info 508 2857697280;Sector ($start+1) $info;Sector ($start+7) $info
    $fat=New-Object byte[] 512;Put32 $fat 0 268435448;Put32 $fat 4 268435455;foreach($cluster in @(2,4,5,6,7)){Put32 $fat ($cluster*4) 268435455};Put32 $fat 12 5;Sector ($start+32) $fat;Sector ($start+1056) $fat
    $data=$start+2080
    $root=New-Object byte[] 512;Entry $root 0 'README  TXT' 3 700;Entry $root 32 'DIR        ' 4 0 16
    $short=[Text.Encoding]::ASCII.GetBytes('LONGFI~1TXT');$checksum=0;foreach($c in $short){$checksum=(([int]($checksum-band 1)*128)+($checksum-shr 1)+$c)-band 255}
    $units=[char[]]'Long file name.txt';$positions=@(1,3,5,7,9,14,16,18,20,22,24,28,30)
    for($ordinal=2;$ordinal-ge 1;$ordinal--){$offset=64+(2-$ordinal)*32;$root[$offset]=$ordinal;if($ordinal-eq 2){$root[$offset]=$ordinal-bor 64};$root[$offset+11]=15;$root[$offset+13]=$checksum
        for($j=0;$j-lt 13;$j++){$index=($ordinal-1)*13+$j;$value=65535;if($index-lt $units.Length){$value=[int]$units[$index]}elseif($index-eq $units.Length){$value=0};Put16 $root ($offset+$positions[$j]) $value}}
    Entry $root 128 'LONGFI~1TXT' 7 12;Sector $data $root
    $contents=New-Object byte[] 512;for($i=0;$i-lt 512;$i++){$contents[$i]=$i%251};Sector ($data+1) $contents
    $tail=New-Object byte[] 512;for($i=0;$i-lt 188;$i++){$tail[$i]=($i+512)%251};Sector ($data+3) $tail
    $dir=New-Object byte[] 512;Entry $dir 0 '.          ' 4 0 16;Entry $dir 32 '..         ' 0 0 16;Entry $dir 64 'NESTED  TXT' 6 12;Sector ($data+2) $dir
    $nested=New-Object byte[] 512;Text $nested 0 'nested works';Sector ($data+4) $nested
    $long=New-Object byte[] 512;Text $long 0 'long-name-ok';Sector ($data+5) $long
    $stream.Flush()
} finally {$stream.Dispose()}
Write-Output "Created disposable FAT32 fixture: $target"
