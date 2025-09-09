#!/bin/bash

# Envy Kernel architectural selection script for macOS/Linux

show_menu() {
    echo""
    echo "Envy Kernel Build Menu"
    echo "======================"
    echo "Please select a build architecture:"
    echo "1. Build for i386"
    echo "2. Build for AMD64"
    echo "3. Build for armv8 (WIP)"
    echo""
    echo "Enter a number from the list: "
}

# Main loop for menu selection
while true; do
    show_menu
    read -r choice

    case $choice in
        1)
            exec "./Compilation/i386.sh"
            break
            ;;
        2)
            exec "./Compilation/amd64.sh"
            break
            ;;
        3)
            exec "./Compilation/armv8.sh"
            break
            ;;
        *)
            echo "Invalid choice. Returning to menu."
            ;;
    esac
done