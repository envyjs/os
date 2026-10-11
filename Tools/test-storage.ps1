param([string]$Qemu='qemu-system-i386',[string]$Iso='Build/Tests/Storage/boot.iso')
$ErrorActionPreference='Stop'
$directory=Join-Path $pwd 'Build/Tests/Storage'
foreach($layout in @('mbr','superfloppy')) {
    $image=Join-Path $directory "fat32-$layout.img"
    & "$PSScriptRoot/prepare-storage-test.ps1" -Output $image -Superfloppy:($layout-eq 'superfloppy')
    foreach($machine in @('pc','q35')) {
        $log=Join-Path $directory "$machine-$layout.log"
        Write-Output "Testing AHCI DMA and FAT32: $machine / $layout"
        & $Qemu -machine $machine -m 256 -display none -monitor none -serial none -debugcon "file:$log" -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=4 -drive "file=$Iso,format=raw,if=ide,index=2,media=cdrom,readonly=on" -device ich9-ahci,id=sata -drive "file=$image,format=raw,if=none,id=fatdisk,snapshot=on" -device ide-hd,drive=fatdisk,bus=sata.0 -boot d
        if($LASTEXITCODE-ne 33){if(Test-Path -LiteralPath $log){Get-Content -LiteralPath $log};throw "Storage guest failed with exit code $LASTEXITCODE"}
        Get-Content -LiteralPath $log
    }
}
Write-Output 'SATA/FAT32 storage tests passed.'
