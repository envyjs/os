#!/bin/bash

# Envy Kernel architectural selection script for macOS/Linux

if [[ "$OSTYPE" == "msys"* ]]; then
    echo "Windows is not supported, quitting."
    exit 1
fi

show_menu() {
    echo""
    echo "Envy Kernel Build Menu"
    echo "======================"
    echo "Please select a build architecture:"
    echo "1. Build for i386"
    echo "2. Build for AMD64"
    echo "3. Build for armv8 (WIP)"
    echo "4. Build for armv8 with Android (WIP)"
    echo""
    echo "Enter a number from the list: "
}

# Main loop for menu selection
while true; do
    show_menu
    read -r choice

    case $choice in
        1)
            mkdir ../Output/bin/i386/
            exec "./Compilation/i386.sh"
            break
            ;;
        2)
            mkdir ../Output/bin/AMD64/
            exec "./Compilation/amd64.sh"
            break
            ;;
        3)
            mkdir ../Output/bin/armv8/
            exec "./Compilation/armv8.sh"
            break
            ;;

        4)
            cd ../System/armv8/Compat/
            echo "!! CAUTION !!"
            echo "This will download a large amount of data (around 100GB) from the Android source repository."
            echo "Make sure you have sufficient disk space and a stable internet connection."
            echo "This process may take a long time (up to 5 hours!) depending on your internet speed."
            read -p "Continue? (y/n): " yn
            case $yn in
                [Yy]* ) 
                exec repo init -u https://android.googlesource.com/platform/manifest -b android-14.0.0_r30
                exec repo sync -j$(nproc)
                exec "./Compilation/armv8.sh"
                ;;
                [Nn]* ) echo "Aborted."; exit 1 ;;
                * ) echo "Please answer yes or no."; exit 1 ;;
            esac
            ;;
        *)
            echo "Invalid choice. Returning to menu."
            ;;
    esac
done